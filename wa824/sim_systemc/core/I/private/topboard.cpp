#include <systemc.h>
#include "topboard.h"

// ctrl 비트 인덱스 매크로
#define CTRL_ADDR2_SEL  0
#define CTRL_ADDR3_SEL  1
#define CTRL_ALU_B_SEL  2
#define CTRL_IMME_SEL   3
// [7:4] alu_funct
#define CTRL_MEM_READ   8
#define CTRL_MEM_WRITE  9
#define CTRL_WE4L       10
#define CTRL_WE4H       11
// [13:12] wb_sel

static inline bool ctrl_bit(const sc_lv<14>& c, int i) {
	return c[i] == SC_LOGIC_1;
}
static inline unsigned int ctrl_range(const sc_lv<14>& c, int hi, int lo) {
	return c.range(hi, lo).to_uint();
}

// 16비트 부호확장 헬퍼
static sc_lv<16> sext8(sc_lv<8> v) {
	sc_int<8> sv = v.to_uint();
	return sc_lv<16>((sc_int<16>)sv);
}
static sc_lv<16> sext12(sc_lv<12> v) {
	sc_int<12> sv = v.to_uint();
	return sc_lv<16>((sc_int<16>)sv);
}

// ─────────────────────────────────────────────────────────────────────────────

thpg::wa824::Topboard::Topboard(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	// ── SC_METHOD 등록 ────────────────────────────────────────
	SC_METHOD(tick_debug); sensitive << clk.pos();
	SC_METHOD(tick_pc);    sensitive << clk.pos();
	SC_METHOD(tick_if_id); sensitive << clk.pos();
	SC_METHOD(tick_id_ex); sensitive << clk.pos();
	SC_METHOD(tick_ex_mem);sensitive << clk.pos();
	SC_METHOD(tick_mem_wb);sensitive << clk.pos();

	SC_METHOD(debug_invalidate);
	sensitive << dbg_ic_invalidate << dbg_ic_invalidate_addr << dbg_ic_invalidate_all
	          << dbg_dc_invalidate << dbg_dc_invalidate_addr << dbg_dc_invalidate_all;

	SC_METHOD(comb_if);
	sensitive << reset
	          << pc_reg << if_inst_sig << ic_hit_sig
	          << fc_jump_type << fc_b_offset << fc_jr_imme << fc_ja_imme
	          << bpe_result << not_taken_restore << taken_restore
	          << if_id_inst << if_id_pc << if_id_pc3
	          << immgen_b << id_wrs_data << immgen_jrp
	          << is_jalr_sig;

	SC_METHOD(comb_id);
	sensitive << reset
	          << if_id_inst << if_id_pc << if_id_pc3 << if_id_valid
	          << rf_rs1_raw << rf_rs2_raw << rf_wrs_raw
	          << cu_ctrl << immgen_i << immgen_ls << immgen_jrp
	          << hde_id_rs1_wb_fwd << hde_id_rs2_wb_fwd
	          << hde_id_wrd_fwd_sel << hde_mem_wb_we4h
	          << wb_data_l << wb_data_h
	          << mem_wb_pc3 << mem_wb_alu_out << mem_wb_mem_out
	          << mem_wb_ctrl << mem_wb_rd_sel
	          << ex_mem_alu_out;

	SC_METHOD(comb_ex);
	sensitive << reset
	          << id_ex_rs1 << id_ex_rs2 << id_ex_wrs_data
	          << id_ex_rs1_addr << id_ex_rs2_addr
	          << id_ex_ctrl << id_ex_funct << id_ex_imme
	          << id_ex_opcode
	          << hde_ex_rs1_fwd_sel << hde_ex_rs2_fwd_sel
	          << hde_ex_wrd_fwd_sel
	          << hde_ex_mem_we4h << hde_mem_wb_we4h
	          << ex_mem_alu_out << ex_mem_pc3
	          << wb_data_l << wb_data_h
	          << mem_wb_alu_out << mem_wb_mem_out
	          << id_ex_valid;

	SC_METHOD(comb_mem);
	sensitive << ex_mem_ctrl << ex_mem_addr << ex_mem_rs_store
	          << ex_mem_valid << dc_rdata_sig << dc_hit_sig
	          << mem_wb_ctrl << mem_wb_valid;

	SC_METHOD(comb_wb);
	sensitive << mem_wb_ctrl << mem_wb_alu_out << mem_wb_mem_out
	          << mem_wb_pc3 << mem_wb_rd_sel << mem_wb_valid;



	// ── ICache 연결 ───────────────────────────────────────────
	icache.pc      (pc_reg);
	icache.inst    (if_inst_sig);
	icache.hit     (ic_hit_sig);
	icache.clk     (clk);
	icache.reset   (reset);
	icache.ic_addr (arb_ic_addr);
	icache.ic_re   (arb_ic_re);
	icache.ic_rdata(arb_ic_rdata);
	icache.ic_ready(arb_ic_ready);
	icache.ic_grant(arb_ic_grant);

	// ── DCache 연결 ───────────────────────────────────────────
	dcache.addr     (ex_mem_addr);
	dcache.wdata    (ex_mem_rs_store);
	dcache.mem_read (dc_mem_read);     // comb_mem에서 ex_mem_ctrl[8]로 드라이브
	dcache.mem_write(dc_mem_write);    // comb_mem에서 ex_mem_ctrl[9]로 드라이브
	dcache.rdata    (dc_rdata_sig);
	dcache.hit      (dc_hit_sig);
	dcache.clk      (clk);
	dcache.reset    (reset);
	dcache.mmio_enable (mmio_enable);
	dcache.mmio_base   (mmio_base);
	dcache.mmio_mask   (mmio_mask);
	dcache.mmio_ordered(mmio_ordered);
	dcache.dc_addr  (arb_dc_addr);
	dcache.dc_wdata (arb_dc_wdata);
	dcache.dc_we    (arb_dc_we);
	dcache.dc_re    (arb_dc_re);
	dcache.dc_urgent(arb_dc_urgent);
	dcache.dc_rdata (arb_dc_rdata);
	dcache.dc_ready (arb_dc_ready);
	dcache.dc_grant (arb_dc_grant);

	// ── MemArbiter 연결 ───────────────────────────────────────
	mem_arbiter.mem_addr (mem_addr);
	mem_arbiter.mem_wdata(mem_wdata);
	mem_arbiter.mem_we   (mem_we);
	mem_arbiter.mem_re   (mem_re);
	mem_arbiter.mem_rdata(mem_rdata);
	mem_arbiter.mem_ready(mem_ready);
	mem_arbiter.ic_addr  (arb_ic_addr);
	mem_arbiter.ic_re    (arb_ic_re);
	mem_arbiter.ic_rdata (arb_ic_rdata);
	mem_arbiter.ic_ready (arb_ic_ready);
	mem_arbiter.ic_grant (arb_ic_grant);
	mem_arbiter.dc_addr  (arb_dc_addr);
	mem_arbiter.dc_wdata (arb_dc_wdata);
	mem_arbiter.dc_we    (arb_dc_we);
	mem_arbiter.dc_re    (arb_dc_re);
	mem_arbiter.dc_urgent(arb_dc_urgent);
	mem_arbiter.dc_rdata (arb_dc_rdata);
	mem_arbiter.dc_ready (arb_dc_ready);
	mem_arbiter.dc_grant (arb_dc_grant);

	// ── BPE 연결 ──────────────────────────────────────────────
	// BPE는 IF단에서 현재 fetch 중인 명령어를 예측한다.
	bpe.inst              (if_inst_sig);
	bpe.pc                (pc_reg);
	bpe.branch_pred_result(bpe_result);

	// ── FlowController 연결 ───────────────────────────────────
	fc.inst       (if_inst_sig);
	fc.branch_pred(bpe_result);
	fc.is_jalr    (is_jalr_sig);
	fc.b_offset   (fc_b_offset);
	fc.jr_imme    (fc_jr_imme);
	fc.ja_imme    (fc_ja_imme);
	fc.jump_type  (fc_jump_type);

	// ── ControlUnit 연결 ──────────────────────────────────────
	cu.inst(if_id_inst);
	cu.ctrl(cu_ctrl);

	// ── ImmGen 연결 ───────────────────────────────────────────
	immgen.inst    (if_id_inst);
	immgen.imme_i  (immgen_i);
	immgen.imme_b  (immgen_b);
	immgen.imme_ls (immgen_ls);
	immgen.imme_jrp(immgen_jrp);

	// ── RegisterFile 연결 ─────────────────────────────────────
	// rs1_addr: inst[15:12] 고정 → rf_addr1 (comb_id 드라이브)
	// rs2_addr: addr2_sel mux  → rf_addr2_mux (comb_id 드라이브)
	// wrs_addr: addr3_sel mux  → rf_addr3_mux (comb_id 드라이브)
	regfile.rs1_addr(rf_addr1);
	regfile.rs2_addr(rf_addr2_mux);
	regfile.wrs_addr(rf_addr3_mux);
	regfile.rs1_data(rf_rs1_raw);
	regfile.rs2_data(rf_rs2_raw);
	regfile.wrs_data(rf_wrs_raw);
	regfile.rd_addr  (mem_wb_rd_sel);
	regfile.rd_data_l(wb_data_l);
	regfile.rd_data_h(wb_data_h);
	regfile.rd_we_l  (wb_we4l);
	regfile.rd_we_h  (wb_we4h);
	regfile.clk      (clk);
	regfile.reset    (reset);

	// ── BranchComparator 연결 (ID단) ──────────────────────────
	branch_cmp.rs1  (id_rs1_data);
	branch_cmp.rs2  (id_rs2_data);
	branch_cmp.funct(id_branch_funct);  // if_id_inst[19:16] (comb_id에서 드라이브)
	branch_cmp.taken(branch_taken);

	// ── ALU 연결 ─────────────────────────────────────────────
	alu.a       (ex_rs1_data);
	alu.b       (ex_alu_b);
	alu.funct   (id_ex_funct);
	alu.carry_in(alu_carry_in);  // 추후 flags carry 연결 (현재 항상 0)
	alu.y       (alu_out_sig);
	alu.flags   (alu_flags);

	// ── HazardEngine 연결 ─────────────────────────────────────
	hde.clk               (clk);
	hde.reset             (reset);
	hde.icache_hit        (ic_hit_sig);
	hde.dcache_hit        (dc_hit_sig);
	hde.branch_pred_result(bpe_result);
	hde.branch_result     (branch_taken);  // ID단 BranchComparator 실제 결과
	hde.is_jalr           (is_jalr_sig);
	hde.inst              (if_id_inst);
	hde.if_inst           (if_inst_sig);
	hde.if_pc             (pc_reg);
	hde.id_ex_ctrl        (id_ex_ctrl);
	hde.id_ex_rd          (id_ex_rd_sel);
	hde.id_ex_rs1_addr    (id_ex_rs1_addr);
	hde.id_ex_rs2_addr    (id_ex_rs2_addr);
	hde.id_ex_opcode      (id_ex_opcode);
	hde.id_ex_wrs         (id_ex_wrs_addr);
	hde.ex_mem_ctrl       (ex_mem_ctrl);
	hde.ex_mem_rd         (ex_mem_rd_sel);
	hde.ex_mem_we4l       (hde_ex_mem_we4l);
	hde.ex_mem_we4h       (hde_ex_mem_we4h);
	hde.mem_wb_rd         (mem_wb_rd_sel);
	hde.mem_wb_we4l       (hde_mem_wb_we4l);
	hde.mem_wb_we4h       (hde_mem_wb_we4h);
	hde.pc_stall          (hde_pc_stall);
	hde.if_id_stall       (hde_if_id_stall);
	hde.if_id_flush       (hde_if_id_flush);
	hde.not_taken_restore (not_taken_restore);
	hde.taken_restore     (taken_restore);
	hde.id_ex_stall       (hde_id_ex_stall);
	hde.id_ex_flush       (hde_id_ex_flush);
	hde.ex_mem_stall      (hde_ex_mem_stall);
	hde.ex_mem_flush      (hde_ex_mem_flush);
	hde.mem_wb_stall      (hde_mem_wb_stall);
	hde.mem_wb_flush      (hde_mem_wb_flush);
	hde.id_rs1_wb_fwd     (hde_id_rs1_wb_fwd);
	hde.id_rs2_wb_fwd     (hde_id_rs2_wb_fwd);
	hde.ex_rs1_wb_fwd_sel (hde_ex_rs1_fwd_sel);
	hde.ex_rs2_wb_fwd_sel (hde_ex_rs2_fwd_sel);
	hde.id_wrd_fwd_sel    (hde_id_wrd_fwd_sel);
	hde.ex_wrd_fwd_sel    (hde_ex_wrd_fwd_sel);
}

// ─────────────────────────────────────────────────────────────────────────────
// tick: 파이프라인 레지스터
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::tick_debug(void)
{
	++m_cycle;
}

void thpg::wa824::Topboard::debug_invalidate(void)
{
	if (dbg_ic_invalidate_all.read()) {
		icache.invalidate_all();
	} else if (dbg_ic_invalidate.read() && dbg_ic_invalidate_addr.read().is_01()) {
		icache.invalidate_line(
		    static_cast<uint16_t>(dbg_ic_invalidate_addr.read().to_uint()));
	}

	if (dbg_dc_invalidate_all.read()) {
		dcache.invalidate_all();
	} else if (dbg_dc_invalidate.read() && dbg_dc_invalidate_addr.read().is_01()) {
		dcache.invalidate_line(
		    static_cast<uint16_t>(dbg_dc_invalidate_addr.read().to_uint()));
	}
}

bool thpg::wa824::Topboard::debug_read_byte(uint16_t address, uint8_t &value) const
{
	return dcache.debug_peek_byte(address, value);
}

void thpg::wa824::Topboard::tick_pc(void)
{
	if (reset.read()) {
		pc_reg.write(sc_lv<16>("0000000000000000"));
		return;
	}
	if (hde_pc_stall.read()) return;

	pc_reg.write(next_pc.read());
}

void thpg::wa824::Topboard::tick_if_id(void)
{
	if (reset.read() || hde_if_id_flush.read()) {
		if_id_valid.write(false);
		if_id_inst.write(sc_lv<24>(SC_LOGIC_0));
		if_id_pc.write(sc_lv<16>(SC_LOGIC_0));
		if_id_pc3.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}
	if (hde_if_id_stall.read()) return;

	if_id_valid.write(ic_hit_sig.read());
	if_id_inst.write(if_inst_sig.read());
	if_id_pc.write(pc_reg.read());
	// PC+3
	sc_uint<16> p3 = pc_reg.read().to_uint() + 3;
	if_id_pc3.write(sc_lv<16>(p3));
}

void thpg::wa824::Topboard::tick_id_ex(void)
{
	if (reset.read() || hde_id_ex_flush.read()) {
		id_ex_valid.write(false);
		id_ex_ctrl.write(sc_lv<14>(SC_LOGIC_0));
		id_ex_rd_sel.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_rs1_addr.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_rs2_addr.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_wrs_addr.write(sc_lv<3>(SC_LOGIC_0));
		id_ex_rs1.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_rs2.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_rs_store.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_wrs_data.write(sc_lv<16>(SC_LOGIC_0));
		id_ex_imme.write(sc_lv<12>(SC_LOGIC_0));
		id_ex_opcode.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_funct.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_pc.write(sc_lv<16>(SC_LOGIC_0));
		id_ex_pc3.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}
	if (hde_id_ex_stall.read()) return;

	id_ex_valid.write(if_id_valid.read());
	id_ex_ctrl.write(cu_ctrl.read());
	// rd_sel: 일반 8bit write는 inst[3:0]의 바이트 레지스터 주소를 쓴다.
	// JA/JR/JRP의 WRD는 inst[3:1]의 3bit 페어 번호이므로 RF 내부의
	// 바이트 주소(x0l=0, x0h=1, x1l=2, ...)로 변환하려면 <<1 해야 한다.
	sc_lv<4> rd_sel = if_id_inst.read().range(3, 0);
	if (if_id_valid.read() && if_id_inst.read().is_01()) {
		const unsigned int op = if_id_inst.read().range(7, 4).to_uint();
		if (op == 0x5 || op == 0x6 || op == 0x7) {
			rd_sel = sc_lv<4>(if_id_inst.read().range(3, 1).to_uint() << 1);
		}
	}
	id_ex_rd_sel.write(rd_sel);
	// EX단 forwarding은 ID단에서 latch한 실제 소스 주소를 사용한다.
	id_ex_rs1_addr.write(rf_addr1.read());
	id_ex_rs2_addr.write(rf_addr2_mux.read());
	id_ex_wrs_addr.write(rf_addr3_mux.read());
	id_ex_rs1.write(id_rs1_data.read());
	id_ex_rs2.write(id_rs2_data.read());
	id_ex_rs_store.write(id_rs2_data.read()); // S-Type store data
	id_ex_wrs_data.write(id_wrs_data.read());
	id_ex_imme.write(id_imme_sel.read());
	id_ex_opcode.write(if_id_inst.read().range(7, 4));
	id_ex_funct.write(if_id_inst.read().range(19, 16));
	id_ex_pc.write(if_id_pc.read());
	id_ex_pc3.write(if_id_pc3.read());
}

void thpg::wa824::Topboard::tick_ex_mem(void)
{
	if (reset.read() || hde_ex_mem_flush.read()) {
		ex_mem_valid.write(false);
		ex_mem_ctrl.write(sc_lv<14>(SC_LOGIC_0));
		ex_mem_rd_sel.write(sc_lv<4>(SC_LOGIC_0));
		ex_mem_alu_out.write(sc_lv<8>(SC_LOGIC_0));
		ex_mem_addr.write(sc_lv<16>(SC_LOGIC_0));
		ex_mem_rs_store.write(sc_lv<8>(SC_LOGIC_0));
		ex_mem_pc.write(sc_lv<16>(SC_LOGIC_0));
		ex_mem_pc3.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}
	if (hde_ex_mem_stall.read()) return;

	ex_mem_valid.write(id_ex_valid.read());
	ex_mem_ctrl.write(id_ex_ctrl.read());
	ex_mem_rd_sel.write(id_ex_rd_sel.read());
	ex_mem_alu_out.write(alu_out_sig.read());
	ex_mem_addr.write(ex_addr_out.read());
	ex_mem_rs_store.write(id_ex_rs_store.read());
	ex_mem_pc.write(id_ex_pc.read());
	ex_mem_pc3.write(id_ex_pc3.read());
}

void thpg::wa824::Topboard::tick_mem_wb(void)
{
	if (reset.read() || hde_mem_wb_flush.read()) {
		mem_wb_valid.write(false);
		mem_wb_ctrl.write(sc_lv<14>(SC_LOGIC_0));
		mem_wb_rd_sel.write(sc_lv<4>(SC_LOGIC_0));
		mem_wb_alu_out.write(sc_lv<8>(SC_LOGIC_0));
		mem_wb_mem_out.write(sc_lv<8>(SC_LOGIC_0));
		mem_wb_pc3.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}
	if (hde_mem_wb_stall.read()) return;

	mem_wb_valid.write(ex_mem_valid.read());
	mem_wb_ctrl.write(ex_mem_ctrl.read());
	mem_wb_rd_sel.write(ex_mem_rd_sel.read());
	mem_wb_alu_out.write(ex_mem_alu_out.read());
	mem_wb_mem_out.write(dc_rdata_sig.read());
	mem_wb_pc3.write(ex_mem_pc3.read());
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_if: PC mux + next PC 계산
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_if(void)
{
	// reset 중에는 아직 초기화되지 않은 4-value 신호를 정수로 변환하지 않는다.
	if (reset.read()) {
		next_pc.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}

	const sc_lv<16> pc_lv = pc_reg.read();
	if (!pc_lv.is_01()) {
		next_pc.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}
	sc_uint<16> pc = pc_lv.to_uint();

	// reset 직후에는 FlowController 출력이 아직 X일 수 있다.
	// 4-value 값을 무리하게 정수로 바꾸지 말고 순차 fetch로 처리한다.
	const sc_lv<2> jt_lv = fc_jump_type.read();
	unsigned int jt = jt_lv.is_01() ? jt_lv.to_uint() : 0;

	// BPE는 IF단 조합논리이므로 branch 예측/타깃도 IF 명령어를 기준으로 한다.
	sc_lv<24> if_inst = if_inst_sig.read();
	unsigned int if_opcode = if_inst.is_01()
	                       ? if_inst.range(7, 4).to_uint() : 0xFF;

	// ── Branch prediction 복구 (ID단에서 감지) ────────────────
	// Not-taken restore: 예측=taken, 실제=not-taken → if_id_pc3로 복구
	if (not_taken_restore.read()) {
		next_pc.write(if_id_pc3.read());
		return;
	}
	// Taken restore: 예측=not-taken, 실제=taken → if_id_pc + sext(B-Type imm)
	// branch는 ID단에 있으므로 IF의 fc_b_offset이 아니라 IF/ID 명령어의
	// ImmGen B-Type 출력을 사용한다.
	if (taken_restore.read()) {
		const sc_lv<8>  b_imm = immgen_b.read();
		const sc_lv<16> id_pc = if_id_pc.read();
		if (b_imm.is_01() && id_pc.is_01()) {
			sc_int<16>  off = sc_int<8>(b_imm.to_uint());
			sc_uint<16> cpc =
			    (sc_uint<16>)((sc_int<16>)(id_pc.to_uint()) + off);
			next_pc.write(sc_lv<16>(cpc));
		} else {
			next_pc.write(sc_lv<16>(pc + 3));
		}
		return;
	}

	sc_uint<16> npc = pc + 3; // 기본: 순차

	// jump_type:
	//   00 = branch or normal
	//   01 = JR-Type  (PC + sign_ext(jr_imme))
	//   10 = JRP/JALR (R[WRS] + sign_ext(jrp_imme))
	//   11 = JA-Type  (0x0000 + ja_imme, absolute)
	switch (jt) {
		case 0b00: // B-Type taken or 순차
			if (if_opcode == 0x4 && bpe_result.read()) {
				const sc_lv<8> b_off = fc_b_offset.read();
				if (b_off.is_01()) {
					sc_int<16> off = sc_int<8>(b_off.to_uint());
					npc = (sc_uint<16>)(sc_int<16>(pc) + off);
				}
			}
			// not-taken: PC+3 유지
			break;
		case 0b01: // JR-Type: IF PC + sign_ext(jr_imme)
			{
				const sc_lv<16> jr = fc_jr_imme.read();
				if (jr.is_01()) {
					sc_int<16> off = sc_int<16>(jr.to_uint());
					// JR은 IF단에서 해석된다. 현재 fetch 중인 명령어의 PC 기준.
					npc = (sc_uint<16>)(sc_int<16>(pc) + off);
				}
			}
			break;
		case 0b10: // JRP/JALR: R[WRS] + sign_ext(jrp_imme)
			{
				const sc_lv<16> base_lv = id_wrs_data.read();
				const sc_lv<12> off_lv  = immgen_jrp.read();
				if (base_lv.is_01() && off_lv.is_01()) {
					sc_uint<16> base = base_lv.to_uint();
					sc_int<16>  off  = sc_int<12>(off_lv.to_uint());
					npc = (sc_uint<16>)((sc_int<16>)base + off);
				}
			}
			break;
		case 0b11: // JA-Type: 0x0000 + ja_imme (absolute)
			{
				const sc_lv<16> ja = fc_ja_imme.read();
				if (ja.is_01()) npc = ja.to_uint();
			}
			break;
	}

	next_pc.write(sc_lv<16>(npc));
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_id: addr mux, imme mux, forwarding mux
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_id(void)
{
	if (reset.read()) {
		rf_addr1.write(sc_lv<4>(SC_LOGIC_0));
		rf_addr2_mux.write(sc_lv<4>(SC_LOGIC_0));
		rf_addr3_mux.write(sc_lv<3>(SC_LOGIC_0));
		id_rs1_data.write(sc_lv<8>(SC_LOGIC_0));
		id_rs2_data.write(sc_lv<8>(SC_LOGIC_0));
		id_wrs_data.write(sc_lv<16>(SC_LOGIC_0));
		id_imme_sel.write(sc_lv<12>(SC_LOGIC_0));
		id_branch_funct.write(sc_lv<4>(SC_LOGIC_0));
		is_jalr_sig.write(false);
		return;
	}

	sc_lv<24> inst = if_id_inst.read();

	// addr mux
	// Addr1 (rs1): inst[15:12] 고정
	sc_lv<4> addr1 = inst.is_01() ? sc_lv<4>(inst.range(15, 12)) : sc_lv<4>(SC_LOGIC_0);
	rf_addr1.write(addr1);
	// Addr2 (rs2/rs): addr2_sel=0→inst[11:8], 1→inst[3:0]
	bool a2sel = inst.is_01() && ctrl_bit(cu_ctrl.read(), CTRL_ADDR2_SEL);
	sc_lv<4> addr2 = inst.is_01()
	    ? (a2sel ? sc_lv<4>(inst.range(3, 0)) : sc_lv<4>(inst.range(11, 8)))
	    : sc_lv<4>(SC_LOGIC_0);
	// Addr3 (wrs wide): addr3_sel=0→inst[15:13](L/S), 1→inst[19:17](JRP)
	bool a3sel = inst.is_01() && ctrl_bit(cu_ctrl.read(), CTRL_ADDR3_SEL);
	sc_lv<3> addr3 = inst.is_01()
	    ? (a3sel ? sc_lv<3>(inst.range(19, 17)) : sc_lv<3>(inst.range(15, 13)))
	    : sc_lv<3>(SC_LOGIC_0);

	rf_addr2_mux.write(addr2);
	rf_addr3_mux.write(addr3);

	// WB forwarding (ID RS1/RS2)
	// 8bit write는 데이터가 항상 low bus(rd_data_l)에 실린다. 반면
	// JR/JA/JRP의 16bit pair write는 low/high bus를 모두 사용하므로
	// 읽는 레지스터의 LSB에 따라 high bus를 선택해야 한다.
	auto wb_byte = [&](const sc_lv<4> &addr) -> sc_lv<8> {
		const bool high = addr.is_01() && (addr.to_uint() & 1u);
		if (hde_mem_wb_we4h.read() && high) return wb_data_h.read();
		return wb_data_l.read();
	};
	sc_lv<8> rs1 = hde_id_rs1_wb_fwd.read() ? wb_byte(addr1) : rf_rs1_raw.read();
	sc_lv<8> rs2 = hde_id_rs2_wb_fwd.read() ? wb_byte(addr2) : rf_rs2_raw.read();

	// WRD forwarding (wide)
	unsigned int wrd_sel = hde_id_wrd_fwd_sel.read().to_uint();
	sc_lv<16> wrs;
	if (wrd_sel == 0b10) {
		// EX/MEM forwarding: wide write(JAL/JALR)는 EX단에서 완성되지 않으므로
		// 실제로는 발생하지 않는 케이스. 발생 시 alu_out low만 전달.
		wrs.range(7,  0) = ex_mem_alu_out.read();
		wrs.range(15, 8) = sc_lv<8>(SC_LOGIC_0);
	} else if (wrd_sel == 0b11) {
		// MEM/WB forwarding
		wrs.range(7,  0) = wb_data_l.read();
		wrs.range(15, 8) = wb_data_h.read();
	} else {
		wrs = rf_wrs_raw.read();
	}

	id_rs1_data.write(rs1);
	id_rs2_data.write(rs2);
	id_wrs_data.write(wrs);

	const unsigned int op = inst.is_01()
	                      ? inst.range(7, 4).to_uint() : 0xFu;

	// imme mux: JRP→jrp(12bit), 1→L&S(12bit), 0→I-Type(8bit)
	bool imsel = ctrl_bit(cu_ctrl.read(), CTRL_IMME_SEL);
	if (op == 0x7) {
		id_imme_sel.write(immgen_jrp.read());
	} else if (imsel) {
		// L&S 12bit 그대로 전달
		id_imme_sel.write(immgen_ls.read());
	} else {
		// I-Type 8bit → 12bit sign-extend
		sc_int<8> sv = immgen_i.read().to_uint();
		id_imme_sel.write(sc_lv<12>((sc_int<12>)sv));
	}

	// BranchComparator funct: inst[19:16] (B-Type일 때만 유효)
	id_branch_funct.write(inst.is_01()
	    ? sc_lv<4>(inst.range(19, 16))
	    : sc_lv<4>(SC_LOGIC_X));

	// JRP/JALR(opcode 0x7)은 ID단에서 is_jalr을 만든다. 이 신호와
	// id_wrs_data/immgen_jrp가 같은 stage에서 target을 계산한다.
	is_jalr_sig.write(if_id_valid.read() && inst.is_01() && op == 0x7);
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_ex: ALU, 주소 ADDER, branch fail 감지
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_ex(void)
{
	if (reset.read()) {
		alu_carry_in.write(SC_LOGIC_0);
		ex_rs1_data.write(sc_lv<8>(SC_LOGIC_0));
		ex_rs2_data.write(sc_lv<8>(SC_LOGIC_0));
		ex_alu_b.write(sc_lv<8>(SC_LOGIC_0));
		ex_addr_out.write(sc_lv<16>(SC_LOGIC_0));
		return;
	}

	alu_carry_in.write(SC_LOGIC_0); // 추후 flags carry 연결

	// EX RS1/RS2 forwarding mux
	// 00=레지스터, 10=EX/MEM, 11=MEM/WB
	// JR/JA/JRP는 16bit PC+3을 라이트백하므로 ALU/MEM 결과가 아니라
	// PC+3(pair)을 소스 레지스터의 low/high에 맞춰 전달해야 한다.
	auto fwd8 = [&](unsigned int sel, sc_lv<8> reg_val,
	                const sc_lv<4> &addr) -> sc_lv<8> {
		const bool high = addr.is_01() && (addr.to_uint() & 1u);
		if (sel == 0b10) {
			if (hde_ex_mem_we4h.read()) {
				return high ? sc_lv<8>(ex_mem_pc3.read().range(15, 8))
				            : sc_lv<8>(ex_mem_pc3.read().range(7, 0));
			}
			return ex_mem_alu_out.read();
		}
		if (sel == 0b11) {
			if (hde_mem_wb_we4h.read() && high) return wb_data_h.read();
			return wb_data_l.read();
		}
		return reg_val;
	};

	sc_lv<8> rs1 = fwd8(hde_ex_rs1_fwd_sel.read().to_uint(),
	                    id_ex_rs1.read(), id_ex_rs1_addr.read());
	sc_lv<8> rs2 = fwd8(hde_ex_rs2_fwd_sel.read().to_uint(),
	                    id_ex_rs2.read(), id_ex_rs2_addr.read());
	ex_rs1_data.write(rs1);
	ex_rs2_data.write(rs2);

	// ALU B mux: alu_b_sel=0→RS2, 1→IMME
	sc_lv<8> alu_b = ctrl_bit(id_ex_ctrl.read(), CTRL_ALU_B_SEL)
	               ? sc_lv<8>(id_ex_imme.read().range(7, 0)) : rs2;
	ex_alu_b.write(alu_b);

	// 주소 ADDER: R[WRS] + sign_ext(imme 12bit)
	// L/S-Type: addr = R[WRS] + sext(imm12)
	{
		sc_uint<16> base = id_ex_wrs_data.read().to_uint();
		sc_int<16> off = sc_int<12>(id_ex_imme.read().to_uint());
		sc_uint<16> addr_out = (sc_uint<16>)((sc_int<16>)base + off);
		ex_addr_out.write(sc_lv<16>(addr_out));
	}

}


// ─────────────────────────────────────────────────────────────────────────────
// comb_mem: D-Cache 연결 (mem_read/mem_write 신호 드라이브)
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_mem(void)
{
	// ex_mem_ctrl에서 mem_read/write 비트 추출하여 D-Cache 포트 드라이브
	if (ex_mem_valid.read() && ex_mem_ctrl.read().is_01()) {
		dc_mem_read.write(ctrl_bit(ex_mem_ctrl.read(), CTRL_MEM_READ));
		dc_mem_write.write(ctrl_bit(ex_mem_ctrl.read(), CTRL_MEM_WRITE));
		// HDE용 we4l/h 중간 신호
		hde_ex_mem_we4l.write(ctrl_bit(ex_mem_ctrl.read(), CTRL_WE4L));
		hde_ex_mem_we4h.write(ctrl_bit(ex_mem_ctrl.read(), CTRL_WE4H));
	} else {
		dc_mem_read.write(false);
		dc_mem_write.write(false);
		hde_ex_mem_we4l.write(false);
		hde_ex_mem_we4h.write(false);
	}
	// MEM/WB we4 신호: invalid 단계의 stale ctrl/we4가 forwarding을
	// 잘못 발생시키지 않도록 valid로 게이트한다.
	if (mem_wb_valid.read() && mem_wb_ctrl.read().is_01()) {
		hde_mem_wb_we4l.write(ctrl_bit(mem_wb_ctrl.read(), CTRL_WE4L));
		hde_mem_wb_we4h.write(ctrl_bit(mem_wb_ctrl.read(), CTRL_WE4H));
	} else {
		hde_mem_wb_we4l.write(false);
		hde_mem_wb_we4h.write(false);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_wb: WB mux → RegFile write data
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_wb(void)
{
	if (!mem_wb_valid.read() || !mem_wb_ctrl.read().is_01()) {
		wb_data_l.write(sc_lv<8>(SC_LOGIC_0));
		wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
		wb_we4l.write(false);
		wb_we4h.write(false);
		return;
	}

	bool we4l = ctrl_bit(mem_wb_ctrl.read(), CTRL_WE4L);
	bool we4h = ctrl_bit(mem_wb_ctrl.read(), CTRL_WE4H);
	wb_we4l.write(we4l);
	wb_we4h.write(we4h);

	unsigned int wb_sel = ctrl_range(mem_wb_ctrl.read(), 13, 12);
	sc_lv<16> pc3 = mem_wb_pc3.read();

	switch (wb_sel) {
		case 0b00: // alu_out (8bit, we4l만)
			wb_data_l.write(mem_wb_alu_out.read());
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
		case 0b01: // PC+3 (JAL/JALR return addr, 16bit, we4l+we4h)
			wb_data_l.write(sc_lv<8>(pc3.range(7,  0)));
			wb_data_h.write(sc_lv<8>(pc3.range(15, 8)));
			break;
		case 0b10: // mem_out (Load, 8bit, we4l만)
			wb_data_l.write(mem_wb_mem_out.read());
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
		default:
			wb_data_l.write(sc_lv<8>(SC_LOGIC_0));
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
	}
}
