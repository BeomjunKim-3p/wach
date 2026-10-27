#include <systemc.h>
#include "hazard_engine.h"

thpg::wa824::HazardEngine::HazardEngine(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(tick);
	sensitive << clk.pos();

	SC_METHOD(refresh);
	sensitive
		<< reset
		<< icache_hit << dcache_hit
		<< branch_pred_result << branch_result
		<< is_jalr
		<< inst
		<< if_inst << if_pc
		<< id_ex_ctrl << id_ex_rd << id_ex_wrs
		<< id_ex_rs1_addr << id_ex_rs2_addr << id_ex_opcode
		<< ex_mem_ctrl << ex_mem_rd << ex_mem_we4l << ex_mem_we4h
		<< mem_wb_rd << mem_wb_we4l << mem_wb_we4h;
}

void thpg::wa824::HazardEngine::tick(void)
{
	m_prev_inst = inst.read();

	if (reset.read()) {
		m_pred_valid = false;
		m_pred_taken = false;
		m_pred_pc    = sc_lv<16>(SC_LOGIC_0);
		return;
	}

	// 실제 비교가 일어나는 ID단 branch. I/D-Cache miss로 IF/ID가 멈춰 있으면
	// branch도 아직 해소된 것이 아니므로 예측을 소비하지 않는다.
	// inst.read()이 유효한지 확인 후 비트 필드로 디코드
	const uint32_t cur_inst_word = inst.read().to_uint();
	bool branch_in_id = inst.read().is_01() &&  ((cur_inst_word >> 4) & 0x0F) == 0x4;
	bool actual_valid = branch_result.read().is_01();
	// ID단 branch의 소스가 아직 파이프라인에 있으면 branch_result는 아직
	// 확정된 값이 아니다. 이 사이클에는 예측을 소비하지 않는다.
	bool blocked = detect_dcache_miss() || detect_icache_miss() ||
	               detect_branch_source_hazard();
	bool resolved = branch_in_id && actual_valid && m_pred_valid && !blocked;

	// IF단에서 새로 예측한 branch를 캡처한다. flush 사이클의 IF는 잘못된
	// 경로이므로 캡처하지 않는다.
	bool if_valid = icache_hit.read() && if_inst.read().is_01();
	bool if_branch = if_valid && if_inst.read().range(7, 4).to_uint() == 0x4;
	bool flush = if_id_flush.read();
	bool capture = if_branch && !flush;

	if (resolved) {
		m_pred_valid = false;
	}

	// branch가 아닌 제어 흐름(JALR 등)으로 IF/ID가 flush되면 대기 중인 예측도 버린다.
	if (flush && !resolved) {
		m_pred_valid = false;
	}

	if (capture) {
		m_pred_valid = true;
		m_pred_taken = branch_pred_result.read();
		m_pred_pc    = if_pc.read();
	}
}

bool thpg::wa824::HazardEngine::detect_icache_miss(void) const
{
	return !icache_hit.read();
}

bool thpg::wa824::HazardEngine::detect_dcache_miss(void) const
{
	bool mem_active = ex_mem_ctrl.read().is_01() &&
	                  (ex_mem_ctrl.read()[8] == SC_LOGIC_1 ||  // mem_read
                   ex_mem_ctrl.read()[9] == SC_LOGIC_1);   // mem_write
	return mem_active && !dcache_hit.read();
}

bool thpg::wa824::HazardEngine::detect_not_taken_restore(void) const
{
	// ID단에 도착한 B-Type과 IF에서 캡처해 둔 예측을 비교한다.
	if (!m_pred_valid || !m_pred_taken) return false;
	if (!inst.read().is_01()) return false;
	if (((inst.read().to_uint() >> 4) & 0x0F) != 0x4) return false;
	if (!branch_result.read().is_01()) return false;
	// 예측=taken, 실제=not-taken
	return branch_result.read() == SC_LOGIC_0;
}

bool thpg::wa824::HazardEngine::detect_taken_restore(void) const
{
	if (!m_pred_valid || m_pred_taken) return false;
	if (!inst.read().is_01()) return false;
	if (((inst.read().to_uint() >> 4) & 0x0F) != 0x4) return false;
	if (!branch_result.read().is_01()) return false;
	// 예측=not-taken, 실제=taken
	return branch_result.read() == SC_LOGIC_1;
}

bool thpg::wa824::HazardEngine::detect_jalr(void) const
{
	return is_jalr.read();
}

bool thpg::wa824::HazardEngine::detect_branch_source_hazard(void) const
{
	// B-Type은 ID단에서 비교하므로, producer가 파이프라인에서 완전히 빠질
	// 때까지 branch를 stall한다. MEM/WB 단계에서도 한 사이클 더 기다려
	// WB forwarding과 register file write가 안정된 뒤 branch_result를 쓴다.
	if (!inst.read().is_01()) return false;
	if (((inst.read().to_uint() >> 4) & 0x0F) != 0x4) return false;

	const unsigned int rs1 = ((inst.read().to_uint() >> 12) & 0x0F);
	const unsigned int rs2 = ((inst.read().to_uint() >> 0) & 0x0F);

	auto matches = [&](unsigned int index, bool we4l, bool we4h) {
		// we4h는 rd의 LSB를 1로 강제한 주소에 쓴다.
		const unsigned int high_index = index | 1u;
		return (we4l && (index == rs1 || index == rs2)) ||
		       (we4h && (high_index == rs1 || high_index == rs2));
	};
	auto writes_source = [&](const sc_lv<14> &ctrl, const sc_lv<4> &rd) {
		if (!ctrl.is_01() || !rd.is_01()) return false;
		return matches(rd.to_uint(),
		               ctrl[10] == SC_LOGIC_1,
		               ctrl[11] == SC_LOGIC_1);
	};

	if (writes_source(id_ex_ctrl.read(), id_ex_rd.read())) return true;
	if (writes_source(ex_mem_ctrl.read(), ex_mem_rd.read())) return true;
	if (mem_wb_rd.read().is_01()) {
		return matches(mem_wb_rd.read().to_uint(),
		               mem_wb_we4l.read(),
		               mem_wb_we4h.read());
	}
	return false;
}

bool thpg::wa824::HazardEngine::detect_load_use(void) const
{
	// ID/EX 단에 Load 명령 있고, 현재 IF/ID 단 명령어가 그 rd를 rs1 또는 rs2로 사용
	if (!id_ex_ctrl.read().is_01()) return false;
	if (id_ex_ctrl.read()[8] != SC_LOGIC_1) return false; // id_ex mem_read
	if (!id_ex_rd.read().is_01()) return false;

	if (!inst.read().is_01()) return false;
	const uint32_t word = inst.read().to_uint();
	unsigned int op = (word >> 4) & 0x0F;
	// 현재 명령어가 실제로 읽는 소스 레지스터만 검사한다.
	bool check_rs1 = (op == 0x0 || op == 0x1 || op == 0x4);
	bool check_rs2 = (op == 0x0 || op == 0x3 || op == 0x4);

	// ISA.ko-KR.MD (LSB 0 기준)
	//   R  : RS1[15:12], RS2[11:8]
	//   I  : RS[15:12]
	//   S  : RS[3:0]
	//   B  : RS1[15:12], RS2[3:0]
	unsigned int rs1 = 0;
	unsigned int rs2 = 0;
	if (op == 0x0) {          // R-Type
		rs1 = (word >> 12) & 0x0F;
		rs2 = (word >> 8) & 0x0F;
	} else if (op == 0x1) {   // I-Type
		rs1 = (word >> 12) & 0x0F;
	} else if (op == 0x3) {   // S-Type
		rs2 = word & 0x0F;
	} else if (op == 0x4) {   // B-Type
		rs1 = (word >> 12) & 0x0F;
		rs2 = word & 0x0F;
	}

	return (check_rs1 && id_ex_rd.read().to_uint() == rs1) ||
	       (check_rs2 && id_ex_rd.read().to_uint() == rs2);
}

bool thpg::wa824::HazardEngine::detect_pair_hazard(void) const
{
	// 직전 명령이 8bit write(we4l=1, we4h=0)이고
	// 현재 명령이 wide read(wrs)로 같은 페어를 읽으려 할 때
	if (!id_ex_ctrl.read().is_01()) return false;
	bool prev_we4l = id_ex_ctrl.read()[10] == SC_LOGIC_1;
	bool prev_we4h = id_ex_ctrl.read()[11] == SC_LOGIC_1;
	bool prev_8bit_write = prev_we4l ^ prev_we4h;
	if (!prev_8bit_write) return false;

	if (!inst.read().is_01()) return false;

	// opcode로 wide read 사용 여부 판별: L(0x2), S(0x3), JRP(0x7)
	unsigned int op = ((inst.read().to_uint() >> 4) & 0x0F);
	bool uses_wide = (op == 0x2 || op == 0x3 || op == 0x7);
	if (!uses_wide) return false;

	if (!id_ex_rd.read().is_01()) return false;

	// WRS: L/S는 inst[15:13], JRP는 inst[19:17]
	const uint32_t word = inst.read().to_uint();
	unsigned int wrs_pair = (op == 0x7)
	                      ? ((word >> 17) & 0x07)
	                      : ((word >> 13) & 0x07);
	unsigned int rd_pair  = id_ex_rd.read().to_uint() >> 1;

	return rd_pair == wrs_pair;
}

void thpg::wa824::HazardEngine::calc_forwarding(
	bool &id_rs1_fwd, bool &id_rs2_fwd,
	unsigned int &ex_rs1_sel, unsigned int &ex_rs2_sel,
	unsigned int &id_wrd_sel,  unsigned int &ex_wrd_sel) const
{
	id_rs1_fwd  = false;
	id_rs2_fwd  = false;
	ex_rs1_sel  = 0b00;
	ex_rs2_sel  = 0b00;
	id_wrd_sel  = 0b00;
	ex_wrd_sel  = 0b00;

	if (!ex_mem_rd.read().is_01() || !mem_wb_rd.read().is_01()) return;

	const unsigned int ex_rd = ex_mem_rd.read().to_uint();
	const unsigned int wb_rd = mem_wb_rd.read().to_uint();
	const bool ex_we4l = ex_mem_we4l.read();
	const bool ex_we4h = ex_mem_we4h.read();
	const bool wb_we4l = mem_wb_we4l.read();
	const bool wb_we4h = mem_wb_we4h.read();

	auto wb_matches = [&](unsigned int rs) {
		return (wb_we4l && wb_rd == rs) ||
		       (wb_we4h && (wb_rd | 1) == rs);
	};
	auto ex_matches = [&](unsigned int rs) {
		return (ex_we4l && ex_rd == rs) ||
		       (ex_we4h && (ex_rd | 1) == rs);
	};
	auto pair_matches = [](unsigned int rd, bool we4l, bool we4h,
	                       unsigned int wrs) {
		return (we4l && (rd >> 1) == wrs) ||
		       (we4h && (rd >> 1) == wrs);
	};

	// ── ID 단 (IF/ID): WB forwarding + wide read ──────────────
	if (inst.read().is_01()) {
		// ISA.ko-KR.MD (LSB 0 기준) 필드 위치
		//   R  : RS1[15:12], RS2[11:8]
		//   I  : RS[15:12]
		//   L  : WRS[15:13]
		//   S  : RS[3:0], WRS[15:13]
		//   B  : RS1[15:12], RS2[3:0]
		//   JRP: WRS[19:17]
		const uint32_t word = inst.read().to_uint();
		const unsigned int op = (word >> 4) & 0x0F;

		const bool uses_rs1 = (op == 0x0 || op == 0x1 || op == 0x4);
		const bool uses_rs2 = (op == 0x0 || op == 0x3 || op == 0x4);

		unsigned int rs1 = 0;
		unsigned int rs2 = 0;
		if (op == 0x0) {          // R-Type
			rs1 = (word >> 12) & 0x0F;
			rs2 = (word >> 8) & 0x0F;
		} else if (op == 0x1) {   // I-Type
			rs1 = (word >> 12) & 0x0F;
		} else if (op == 0x3) {   // S-Type (store data)
			rs2 = word & 0x0F;
		} else if (op == 0x4) {   // B-Type
			rs1 = (word >> 12) & 0x0F;
			rs2 = word & 0x0F;
		}

		id_rs1_fwd = uses_rs1 && wb_matches(rs1);
		id_rs2_fwd = uses_rs2 && wb_matches(rs2);

		if (op == 0x2 || op == 0x3 || op == 0x7) {
			const unsigned int wrs = (op == 0x7)
			                       ? ((word >> 17) & 0x07)  // JRP WRS
			                       : ((word >> 13) & 0x07); // L/S WRS
			if (pair_matches(wb_rd, wb_we4l, wb_we4h, wrs)) {
				id_wrd_sel = 0b11;
			}
		}
	}

	// ── EX 단 (ID/EX): latch된 실제 소스 주소 사용 ─────────────
	if (id_ex_rs1_addr.read().is_01() && id_ex_rs2_addr.read().is_01()) {
		const unsigned int rs1 = id_ex_rs1_addr.read().to_uint();
		const unsigned int rs2 = id_ex_rs2_addr.read().to_uint();

		if (wb_matches(rs1)) ex_rs1_sel = 0b11;
		if (ex_matches(rs1)) ex_rs1_sel = 0b10;

		if (wb_matches(rs2)) ex_rs2_sel = 0b11;
		if (ex_matches(rs2)) ex_rs2_sel = 0b10;

		if (id_ex_opcode.read().is_01() && id_ex_wrs.read().is_01()) {
			const unsigned int op = id_ex_opcode.read().to_uint() & 0x0F;
			if (op == 0x2 || op == 0x3 || op == 0x7) {
				const unsigned int wrs = id_ex_wrs.read().to_uint();
				if (pair_matches(wb_rd, wb_we4l, wb_we4h, wrs)) {
					ex_wrd_sel = 0b11;
				}
				if (pair_matches(ex_rd, ex_we4l, ex_we4h, wrs)) {
					ex_wrd_sel = 0b10;
				}
			}
		}
	}
}

// ── 메인 SC_METHOD ────────────────────────────────────────────────────────────

void thpg::wa824::HazardEngine::refresh(void)
{
	bool         id_rs1_fwd = false, id_rs2_fwd = false;
	unsigned int ex_rs1_sel = 0, ex_rs2_sel = 0;
	unsigned int id_wrd_sel = 0, ex_wrd_sel  = 0;
	calc_forwarding(id_rs1_fwd, id_rs2_fwd,
	                ex_rs1_sel, ex_rs2_sel,
	                id_wrd_sel, ex_wrd_sel);

	const bool branch_source_hazard = detect_branch_source_hazard();

	bool pc_s        = false;
	bool ifid_s      = false, ifid_f  = false;
	bool idex_s      = false, idex_f  = false;
	bool exmem_s     = false, exmem_f = false;
	bool memwb_s     = false, memwb_f = false;
	bool not_taken_r = false;
	bool taken_r     = false;

	// ── 우선순위 1: Reset ─────────────────────────────────────
	if (reset.read()) {
		ifid_f = idex_f = exmem_f = memwb_f = true;
		goto done;
	}

	// ── 우선순위 2: D-Cache miss → 전체 stall ────────────────
	if (detect_dcache_miss()) {
		pc_s = ifid_s = idex_s = exmem_s = memwb_s = true;
		goto done;
	}

	// ── 우선순위 3: I-Cache miss → PC+IF_ID stall, ID_EX flush
	if (detect_icache_miss()) {
		pc_s   = true;
		ifid_s = true;
		idex_f = true;
		goto done;
	}

	// ── 우선순위 4: Branch restore → IF_ID flush + restore 신호
	// Branch 소스가 아직 ID/EX 또는 EX/MEM에 있으면 branch_result는
	// 확정되지 않았으므로 복구를 수행하지 않고 아래에서 stall한다.
	if (!branch_source_hazard) {
		if (detect_not_taken_restore()) {
			not_taken_r = true;
			ifid_f = true;
			goto done;
		}
		if (detect_taken_restore()) {
			taken_r = true;
			ifid_f = true;
			goto done;
		}
	}

	// ── 우선순위 5: JRP/JALR → IF_ID flush ──────────────────
	// JR/JA는 IF단에서 해석되므로 여기서 flush하지 않는다. JRP/JALR은
	// ID단에서 target이 결정되므로 IF/ID의 다음 fetch를 버린다.
	if (detect_jalr()) {
		ifid_f = true;
		goto done;
	}

	// ── 우선순위 6: Branch source / Load-to-use / Pair hazard → stall
	if (branch_source_hazard || detect_load_use() || detect_pair_hazard()) {
		pc_s   = true;
		ifid_s = true;
		idex_f = true;
		goto done;
	}

done:
	pc_stall.write(pc_s);
	if_id_stall.write(ifid_s);
	if_id_flush.write(ifid_f);
	id_ex_stall.write(idex_s);
	id_ex_flush.write(idex_f);
	ex_mem_stall.write(exmem_s);
	ex_mem_flush.write(exmem_f);
	mem_wb_stall.write(memwb_s);
	mem_wb_flush.write(memwb_f);
	not_taken_restore.write(not_taken_r);
	taken_restore.write(taken_r);

	id_rs1_wb_fwd.write(id_rs1_fwd);
	id_rs2_wb_fwd.write(id_rs2_fwd);
	ex_rs1_wb_fwd_sel.write(sc_lv<2>(ex_rs1_sel));
	ex_rs2_wb_fwd_sel.write(sc_lv<2>(ex_rs2_sel));
	id_wrd_fwd_sel.write(sc_lv<2>(id_wrd_sel));
	ex_wrd_fwd_sel.write(sc_lv<2>(ex_wrd_sel));
}
