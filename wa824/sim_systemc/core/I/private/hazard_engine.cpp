#include <systemc.h>
#include "hazard_engine.h"

thpg::wa824::HazardEngine::HazardEngine(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive
		<< reset
		<< icache_hit << dcache_hit
		<< branch_pred_result << branch_pred_fail << branch_result
		<< is_jalr
		<< inst
		<< id_ex_ctrl << id_ex_rd << id_ex_wrs
		<< if_id_rs1 << if_id_rs2 << if_id_wrs << if_id_uses_wide
		<< ex_mem_rd << ex_mem_we4l << ex_mem_we4h
		<< mem_wb_rd << mem_wb_we4l << mem_wb_we4h;

	if (tf) {
		std::string mn = std::string(this->name()) + "(HazardEngine)";
		sc_trace(tf, reset,              (mn + ".RESET").c_str());
		sc_trace(tf, icache_hit,         (mn + ".ICACHE_HIT").c_str());
		sc_trace(tf, dcache_hit,         (mn + ".DCACHE_HIT").c_str());
		sc_trace(tf, branch_pred_result, (mn + ".BRANCH_PRED_RESULT").c_str());
		sc_trace(tf, branch_pred_fail,   (mn + ".BRANCH_PRED_FAIL").c_str());
		sc_trace(tf, branch_result,      (mn + ".BRANCH_RESULT").c_str());
		sc_trace(tf, is_jalr,            (mn + ".IS_JALR").c_str());
		sc_trace(tf, inst,               (mn + ".INST").c_str());
		sc_trace(tf, id_ex_ctrl,         (mn + ".ID_EX_CTRL").c_str());
		sc_trace(tf, id_ex_rd,           (mn + ".ID_EX_RD").c_str());
		sc_trace(tf, id_ex_wrs,          (mn + ".ID_EX_WRS").c_str());
		sc_trace(tf, if_id_rs1,          (mn + ".IF_ID_RS1").c_str());
		sc_trace(tf, if_id_rs2,          (mn + ".IF_ID_RS2").c_str());
		sc_trace(tf, if_id_wrs,          (mn + ".IF_ID_WRS").c_str());
		sc_trace(tf, if_id_uses_wide,    (mn + ".IF_ID_USES_WIDE").c_str());
		sc_trace(tf, ex_mem_rd,          (mn + ".EX_MEM_RD").c_str());
		sc_trace(tf, ex_mem_we4l,        (mn + ".EX_MEM_WE4L").c_str());
		sc_trace(tf, ex_mem_we4h,        (mn + ".EX_MEM_WE4H").c_str());
		sc_trace(tf, mem_wb_rd,          (mn + ".MEM_WB_RD").c_str());
		sc_trace(tf, mem_wb_we4l,        (mn + ".MEM_WB_WE4L").c_str());
		sc_trace(tf, mem_wb_we4h,        (mn + ".MEM_WB_WE4H").c_str());
		sc_trace(tf, pc_stall,           (mn + ".PC_STALL").c_str());
		sc_trace(tf, if_id_stall,        (mn + ".IF_ID_STALL").c_str());
		sc_trace(tf, if_id_flush,        (mn + ".IF_ID_FLUSH").c_str());
		sc_trace(tf, id_ex_stall,        (mn + ".ID_EX_STALL").c_str());
		sc_trace(tf, id_ex_flush,        (mn + ".ID_EX_FLUSH").c_str());
		sc_trace(tf, ex_mem_stall,       (mn + ".EX_MEM_STALL").c_str());
		sc_trace(tf, ex_mem_flush,       (mn + ".EX_MEM_FLUSH").c_str());
		sc_trace(tf, mem_wb_stall,       (mn + ".MEM_WB_STALL").c_str());
		sc_trace(tf, mem_wb_flush,       (mn + ".MEM_WB_FLUSH").c_str());
		sc_trace(tf, id_rs1_wb_fwd,      (mn + ".ID_RS1_WB_FWD").c_str());
		sc_trace(tf, id_rs2_wb_fwd,      (mn + ".ID_RS2_WB_FWD").c_str());
		sc_trace(tf, ex_rs1_wb_fwd_sel,  (mn + ".EX_RS1_WB_FWD_SEL").c_str());
		sc_trace(tf, ex_rs2_wb_fwd_sel,  (mn + ".EX_RS2_WB_FWD_SEL").c_str());
		sc_trace(tf, id_wrd_fwd_sel,     (mn + ".ID_WRD_FWD_SEL").c_str());
		sc_trace(tf, ex_wrd_fwd_sel,     (mn + ".EX_WRD_FWD_SEL").c_str());
	}
}

// ── hazard 감지 함수들 ────────────────────────────────────────────────────────

bool thpg::wa824::HazardEngine::detect_icache_miss(void) const
{
	return !icache_hit.read();
}

bool thpg::wa824::HazardEngine::detect_dcache_miss(void) const
{
	// D-Cache miss: mem_read 또는 mem_write 중이고 hit 아닐 때
	bool mem_active = id_ex_ctrl.read().is_01() &&
	                  (id_ex_ctrl.read()[8] == SC_LOGIC_1 ||  // mem_read
	                   id_ex_ctrl.read()[9] == SC_LOGIC_1);   // mem_write
	return mem_active && !dcache_hit.read();
}

bool thpg::wa824::HazardEngine::detect_branch_fail(void) const
{
	return branch_pred_fail.read();
}

bool thpg::wa824::HazardEngine::detect_jalr(void) const
{
	return is_jalr.read();
}

bool thpg::wa824::HazardEngine::detect_load_use(void) const
{
	// ID/EX 단에 Load 명령 있고, 현재 IF/ID 단 명령어가 그 rd를 rs1 또는 rs2로 사용
	if (!id_ex_ctrl.read().is_01()) return false;
	if (id_ex_ctrl.read()[8] != SC_LOGIC_1) return false; // id_ex mem_read
	if (!id_ex_rd.read().is_01()) return false;
	if (!if_id_rs1.read().is_01() || !if_id_rs2.read().is_01()) return false;

	unsigned int rd   = id_ex_rd.read().to_uint();
	unsigned int rs1  = if_id_rs1.read().to_uint();
	unsigned int rs2  = if_id_rs2.read().to_uint();

	return (rd == rs1) || (rd == rs2);
}

bool thpg::wa824::HazardEngine::detect_pair_hazard(void) const
{
	// 직전 명령이 8bit write(we4l=1, we4h=0)이고
	// 현재 명령이 wide read(wrs)로 같은 페어를 읽으려 할 때
	if (!id_ex_ctrl.read().is_01()) return false;
	bool prev_we4l = id_ex_ctrl.read()[10] == SC_LOGIC_1;
	bool prev_we4h = id_ex_ctrl.read()[11] == SC_LOGIC_1;
	bool prev_8bit_write = prev_we4l ^ prev_we4h; // 하나만 켜진 경우 = 8bit write

	if (!prev_8bit_write) return false;
	if (!if_id_uses_wide.read()) return false;
	if (!id_ex_rd.read().is_01() || !if_id_wrs.read().is_01()) return false;

	// rd의 페어 번호(rd[3:1])와 wrs가 같으면 hazard
	unsigned int rd_pair  = id_ex_rd.read().to_uint() >> 1;
	unsigned int wrs_pair = if_id_wrs.read().to_uint();

	return rd_pair == wrs_pair;
}

void thpg::wa824::HazardEngine::calc_forwarding(
	bool &id_rs1_fwd, bool &id_rs2_fwd,
	unsigned int &ex_rs1_sel, unsigned int &ex_rs2_sel,
	unsigned int &id_wrd_sel,  unsigned int &ex_wrd_sel) const
{
	id_rs1_fwd  = false;
	id_rs2_fwd  = false;
	ex_rs1_sel  = 0b00; // 00 = 레지스터 그대로
	ex_rs2_sel  = 0b00;
	id_wrd_sel  = 0b00;
	ex_wrd_sel  = 0b00;

	if (!if_id_rs1.read().is_01() || !if_id_rs2.read().is_01()) return;
	if (!ex_mem_rd.read().is_01() || !mem_wb_rd.read().is_01()) return;

	unsigned int rs1      = if_id_rs1.read().to_uint();
	unsigned int rs2      = if_id_rs2.read().to_uint();
	unsigned int ex_rd    = ex_mem_rd.read().to_uint();
	unsigned int wb_rd    = mem_wb_rd.read().to_uint();
	bool ex_we4l          = ex_mem_we4l.read();
	bool ex_we4h          = ex_mem_we4h.read();
	bool wb_we4l          = mem_wb_we4l.read();
	bool wb_we4h          = mem_wb_we4h.read();

	// ── ID 단 WB forwarding (MEM/WB → ID) ────────────────────
	// rs1
	if ((wb_we4l && wb_rd == rs1) || (wb_we4h && (wb_rd | 1) == rs1))
		id_rs1_fwd = true;
	// rs2
	if ((wb_we4l && wb_rd == rs2) || (wb_we4h && (wb_rd | 1) == rs2))
		id_rs2_fwd = true;

	// ── EX 단 forwarding sel ──────────────────────────────────
	// 우선순위: EX/MEM > MEM/WB
	// ex_rs1_sel: 00=reg, 10=EX/MEM fwd, 11=MEM/WB fwd
	if ((wb_we4l && wb_rd == rs1) || (wb_we4h && (wb_rd | 1) == rs1))
		ex_rs1_sel = 0b11;
	if ((ex_we4l && ex_rd == rs1) || (ex_we4h && (ex_rd | 1) == rs1))
		ex_rs1_sel = 0b10; // EX/MEM 우선

	if ((wb_we4l && wb_rd == rs2) || (wb_we4h && (wb_rd | 1) == rs2))
		ex_rs2_sel = 0b11;
	if ((ex_we4l && ex_rd == rs2) || (ex_we4h && (ex_rd | 1) == rs2))
		ex_rs2_sel = 0b10;

	// ── Wide read (WRD) forwarding ────────────────────────────
	if (!if_id_wrs.read().is_01()) return;
	unsigned int wrs = if_id_wrs.read().to_uint(); // 페어 번호

	// wrs 페어의 l(rd=wrs*2), h(rd=wrs*2+1) 각각이 WB에서 쓰이는지 확인
	auto pair_match = [](unsigned int rd, bool we4l, bool we4h, unsigned int wrs) {
		return (we4l && (rd >> 1) == wrs) || (we4h && (rd >> 1) == wrs);
	};

	if (pair_match(wb_rd, wb_we4l, wb_we4h, wrs))
		id_wrd_sel = 0b11;
	if (pair_match(wb_rd, wb_we4l, wb_we4h, wrs))
		ex_wrd_sel = 0b11;
	if (pair_match(ex_rd, ex_we4l, ex_we4h, wrs))
		ex_wrd_sel = 0b10; // EX/MEM 우선
}

// ── 메인 SC_METHOD ────────────────────────────────────────────────────────────

void thpg::wa824::HazardEngine::refresh(void)
{
	// forwarding 계산 (항상 수행)
	bool         id_rs1_fwd = false, id_rs2_fwd = false;
	unsigned int ex_rs1_sel = 0, ex_rs2_sel = 0;
	unsigned int id_wrd_sel = 0, ex_wrd_sel  = 0;
	calc_forwarding(id_rs1_fwd, id_rs2_fwd,
	                ex_rs1_sel, ex_rs2_sel,
	                id_wrd_sel, ex_wrd_sel);

	// 기본값: 모두 정상 동작
	bool pc_s     = false;
	bool ifid_s   = false, ifid_f  = false;
	bool idex_s   = false, idex_f  = false;
	bool exmem_s  = false, exmem_f = false;
	bool memwb_s  = false, memwb_f = false;

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

	// ── 우선순위 4: Branch fail / JALR → IF_ID flush ─────────
	if (detect_branch_fail() || detect_jalr()) {
		ifid_f = true;
		goto done;
	}

	// ── 우선순위 5: Load-to-use / Pair hazard → stall 1사이클
	if (detect_load_use() || detect_pair_hazard()) {
		pc_s   = true;
		ifid_s = true;
		idex_f = true;
		goto done;
	}

	// ── 우선순위 6: Forwarding only (stall/flush 없음) ────────

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

	id_rs1_wb_fwd.write(id_rs1_fwd);
	id_rs2_wb_fwd.write(id_rs2_fwd);
	ex_rs1_wb_fwd_sel.write(sc_lv<2>(ex_rs1_sel));
	ex_rs2_wb_fwd_sel.write(sc_lv<2>(ex_rs2_sel));
	id_wrd_fwd_sel.write(sc_lv<2>(id_wrd_sel));
	ex_wrd_fwd_sel.write(sc_lv<2>(ex_wrd_sel));
}
