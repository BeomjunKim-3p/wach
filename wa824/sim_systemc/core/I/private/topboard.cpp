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
	SC_METHOD(tick_pc);    sensitive << clk.pos();
	SC_METHOD(tick_if_id); sensitive << clk.pos();
	SC_METHOD(tick_id_ex); sensitive << clk.pos();
	SC_METHOD(tick_ex_mem);sensitive << clk.pos();
	SC_METHOD(tick_mem_wb);sensitive << clk.pos();

	SC_METHOD(comb_if);
	sensitive << pc_reg << icache.inst << icache.hit
	          << fc_jump_type << fc_b_offset << fc_jr_imme << fc_ja_imme
	          << bpe_result << branch_pred_fail << branch_correct_pc
	          << if_id_inst << is_jalr_sig;

	SC_METHOD(comb_id);
	sensitive << if_id_inst << if_id_pc << if_id_pc3 << if_id_valid
	          << rf_rs1_raw << rf_rs2_raw << rf_wrs_raw
	          << cu_ctrl << immgen_i << immgen_ls << immgen_jrp
	          << hde_id_rs1_wb_fwd << hde_id_rs2_wb_fwd
	          << hde_id_wrd_fwd_sel
	          << wb_data_l << wb_data_h
	          << mem_wb_pc3 << mem_wb_alu_out << mem_wb_mem_out
	          << mem_wb_ctrl << mem_wb_rd_sel
	          << ex_mem_alu_out;

	SC_METHOD(comb_ex);
	sensitive << id_ex_rs1 << id_ex_rs2 << id_ex_wrs_data
	          << id_ex_ctrl << id_ex_funct << id_ex_imme
	          << id_ex_imme_b << id_ex_bpred << id_ex_pc
	          << id_ex_opcode
	          << hde_ex_rs1_fwd_sel << hde_ex_rs2_fwd_sel
	          << hde_ex_wrd_fwd_sel
	          << ex_mem_alu_out << mem_wb_alu_out << mem_wb_mem_out
	          << branch_taken << id_ex_valid;

	SC_METHOD(comb_mem);
	sensitive << ex_mem_ctrl << ex_mem_addr << ex_mem_rs_store
	          << ex_mem_valid << dcache.rdata << dcache.hit
	          << ex_mem_ctrl;

	SC_METHOD(comb_wb);
	sensitive << mem_wb_ctrl << mem_wb_alu_out << mem_wb_mem_out
	          << mem_wb_pc3 << mem_wb_rd_sel << mem_wb_valid;

	// ── ICache 연결 ───────────────────────────────────────────
	icache.pc      (pc_reg);
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
	dcache.clk      (clk);
	dcache.reset    (reset);
	dcache.dc_addr  (arb_dc_addr);
	dcache.dc_wdata (arb_dc_wdata);
	dcache.dc_we    (arb_dc_we);
	dcache.dc_re    (arb_dc_re);
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
	mem_arbiter.dc_rdata (arb_dc_rdata);
	mem_arbiter.dc_ready (arb_dc_ready);
	mem_arbiter.dc_grant (arb_dc_grant);

	// ── BPE 연결 ──────────────────────────────────────────────
	bpe.inst              (if_id_inst);
	bpe.pc                (pc_reg);
	bpe.branch_pred_result(bpe_result);

	// ── FlowController 연결 ───────────────────────────────────
	fc.inst       (if_id_inst);
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
	immgen.imme_ls (immgen_ls);
	immgen.imme_ja (immgen_ja);
	immgen.imme_jrp(immgen_jrp);

	// ── RegisterFile 연결 ─────────────────────────────────────
	// rs1_addr: inst[11:8] 고정 → rf_addr1 (comb_id 드라이브)
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
	// we4l/we4h: comb_wb에서 sc_signal로 드라이브 → regfile 연결은 별도 신호 필요
	// 현재는 wb_data_l/h 드라이브와 동일 조건으로 comb_wb에서 처리
	regfile.clk      (clk);
	regfile.reset    (reset);

	// ── BranchComparator 연결 ─────────────────────────────────
	branch_cmp.rs1  (id_rs1_data);
	branch_cmp.rs2  (id_rs2_data);
	branch_cmp.funct(id_ex_funct);
	branch_cmp.taken(branch_taken);

	// ── ALU 연결 ─────────────────────────────────────────────
	alu.a       (ex_rs1_data);
	alu.b       (ex_alu_b);
	alu.funct   (id_ex_funct);
	alu.carry_in(sc_logic(SC_LOGIC_0));  // 추후 flags carry 연결
	alu.y       (alu_out_sig);
	alu.flags   (alu_flags);

	// ── HazardEngine 연결 ─────────────────────────────────────
	hde.reset             (reset);
	hde.icache_hit        (icache.hit);
	hde.dcache_hit        (dcache.hit);
	hde.branch_pred_result(bpe_result);
	hde.branch_pred_fail  (branch_pred_fail);
	hde.branch_result     (branch_taken);
	hde.is_jalr           (is_jalr_sig);
	hde.inst              (if_id_inst);
	hde.id_ex_ctrl        (id_ex_ctrl);
	hde.id_ex_rd          (id_ex_rd_sel);
	hde.id_ex_wrs         (rf_addr3_mux);
	hde.if_id_rs1         (rf_addr2_mux);
	hde.if_id_rs2         (rf_addr2_mux);
	hde.if_id_wrs         (rf_addr3_mux);
	hde.if_id_uses_wide   (uses_wide_sig); // comb_id에서 opcode 디코딩
	hde.ex_mem_rd         (ex_mem_rd_sel);
	hde.ex_mem_we4l       (hde_ex_mem_we4l);
	hde.ex_mem_we4h       (hde_ex_mem_we4h);
	hde.mem_wb_rd         (mem_wb_rd_sel);
	hde.mem_wb_we4l       (hde_mem_wb_we4l);
	hde.mem_wb_we4h       (hde_mem_wb_we4h);
	hde.pc_stall          (hde_pc_stall);
	hde.if_id_stall       (hde_if_id_stall);
	hde.if_id_flush       (hde_if_id_flush);
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
		if_id_bpred.write(false);
		return;
	}
	if (hde_if_id_stall.read()) return;

	if_id_valid.write(icache.hit.read());
	if_id_inst.write(icache.inst.read());
	if_id_pc.write(pc_reg.read());
	// PC+3
	sc_uint<16> p3 = pc_reg.read().to_uint() + 3;
	if_id_pc3.write(sc_lv<16>(p3));
	if_id_bpred.write(bpe_result.read());
}

void thpg::wa824::Topboard::tick_id_ex(void)
{
	if (reset.read() || hde_id_ex_flush.read()) {
		id_ex_valid.write(false);
		id_ex_ctrl.write(sc_lv<14>(SC_LOGIC_0));
		id_ex_rd_sel.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_rs1.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_rs2.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_rs_store.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_wrs_data.write(sc_lv<16>(SC_LOGIC_0));
		id_ex_imme.write(sc_lv<12>(SC_LOGIC_0));
		id_ex_imme_b.write(sc_lv<8>(SC_LOGIC_0));
		id_ex_opcode.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_funct.write(sc_lv<4>(SC_LOGIC_0));
		id_ex_pc.write(sc_lv<16>(SC_LOGIC_0));
		id_ex_pc3.write(sc_lv<16>(SC_LOGIC_0));
		id_ex_bpred.write(false);
		return;
	}
	if (hde_id_ex_stall.read()) return;

	id_ex_valid.write(if_id_valid.read());
	id_ex_ctrl.write(cu_ctrl.read());
	// rd_sel: inst[7:4] (4비트), JA/JR/JRP은 inst[7:5]<<1
	id_ex_rd_sel.write(if_id_inst.read().range(7, 4));
	id_ex_rs1.write(id_rs1_data.read());
	id_ex_rs2.write(id_rs2_data.read());
	id_ex_rs_store.write(id_rs2_data.read()); // S-Type store data
	id_ex_wrs_data.write(id_wrs_data.read());
	id_ex_imme.write(id_imme_sel.read());
	id_ex_opcode.write(if_id_inst.read().range(3, 0));
	id_ex_imme_b.write(fc_b_offset.read());
	id_ex_funct.write(if_id_inst.read().range(23, 20));
	id_ex_pc.write(if_id_pc.read());
	id_ex_pc3.write(if_id_pc3.read());
	id_ex_bpred.write(if_id_bpred.read());

	// is_jalr: 현재 inst가 JALR(opcode=0x7)이면 다음 사이클 IF에 알림
	unsigned int opcode = if_id_inst.read().range(3, 0).to_uint();
	is_jalr_sig.write(opcode == 0x7 && if_id_valid.read());
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
	mem_wb_mem_out.write(dcache.rdata.read());
	mem_wb_pc3.write(ex_mem_pc3.read());
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_if: PC mux + next PC 계산
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_if(void)
{
	sc_uint<16> pc = pc_reg.read().to_uint();
	unsigned int jt = fc_jump_type.read().to_uint();
	unsigned int opcode = if_id_inst.read().is_01()
	                    ? if_id_inst.read().range(3, 0).to_uint() : 0xFF;

	// branch fail 감지 (EX 단에서 계산된 결과 사용)
	if (branch_pred_fail.read()) {
		next_pc.write(branch_correct_pc.read());
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
			if (opcode == 0x4 && bpe_result.read()) {
				sc_int<16> off = sc_int<8>(fc_b_offset.read().to_uint());
				npc = (sc_uint<16>)(sc_int<16>(pc) + off);
			}
			// not-taken: PC+3 유지
			break;
		case 0b01: // JR-Type: PC + sign_ext(jr_imme)
			{
				sc_int<16> off = sc_int<16>(fc_jr_imme.read().to_uint());
				npc = (sc_uint<16>)(sc_int<16>(pc) + off);
			}
			break;
		case 0b10: // JRP/JALR: R[WRS] + sign_ext(jrp_imme)
			{
				sc_uint<16> base = id_wrs_data.read().to_uint();
				sc_int<16>  off  = sc_int<12>(immgen_jrp.read().to_uint());
				npc = (sc_uint<16>)((sc_int<16>)base + off);
			}
			break;
		case 0b11: // JA-Type: 0x0000 + ja_imme (absolute)
			npc = fc_ja_imme.read().to_uint();
			break;
	}

	next_pc.write(sc_lv<16>(npc));
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_id: addr mux, imme mux, forwarding mux
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_id(void)
{
	sc_lv<24> inst = if_id_inst.read();

	// addr mux
	// Addr1 (rs1): inst[11:8] 고정
	sc_lv<4> addr1 = inst.is_01() ? sc_lv<4>(inst.range(11, 8)) : sc_lv<4>(SC_LOGIC_0);
	rf_addr1.write(addr1);
	// Addr2 (rs2/rs): addr2_sel=0→inst[15:12], 1→inst[7:4]
	bool a2sel = inst.is_01() && ctrl_bit(cu_ctrl.read(), CTRL_ADDR2_SEL);
	sc_lv<4> addr2 = inst.is_01()
	    ? (a2sel ? sc_lv<4>(inst.range(7, 4)) : sc_lv<4>(inst.range(15, 12)))
	    : sc_lv<4>(SC_LOGIC_0);
	// Addr3 (wrs wide): addr3_sel=0→inst[11:9], 1→inst[23:21]
	bool a3sel = inst.is_01() && ctrl_bit(cu_ctrl.read(), CTRL_ADDR3_SEL);
	sc_lv<3> addr3 = inst.is_01()
	    ? (a3sel ? sc_lv<3>(inst.range(23, 21)) : sc_lv<3>(inst.range(11, 9)))
	    : sc_lv<3>(SC_LOGIC_0);

	rf_addr2_mux.write(addr2);
	rf_addr3_mux.write(addr3);

	// WB forwarding (ID RS1/RS2)
	sc_lv<8> wb_merged = wb_data_l.read(); // 8bit WB (we4l)
	sc_lv<8> rs1 = hde_id_rs1_wb_fwd.read() ? wb_merged : rf_rs1_raw.read();
	sc_lv<8> rs2 = hde_id_rs2_wb_fwd.read() ? wb_merged : rf_rs2_raw.read();

	// WRD forwarding (wide)
	unsigned int wrd_sel = hde_id_wrd_fwd_sel.read().to_uint();
	sc_lv<16> wrs;
	if (wrd_sel == 0b10) {
		// EX/MEM forwarding: alu_out을 low, 해당 h레지스터 값은 미지원(한계)
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

	// imme mux: 0→I-Type(8bit), 1→L&S-Type(12bit)
	bool imsel = ctrl_bit(cu_ctrl.read(), CTRL_IMME_SEL);
	if (imsel) {
		// L&S 12bit 그대로 전달
		id_imme_sel.write(immgen_ls.read());
	} else {
		// I-Type 8bit → 12bit sign-extend
		sc_int<8> sv = immgen_i.read().to_uint();
		id_imme_sel.write(sc_lv<12>((sc_int<12>)sv));
	}

	// uses_wide: L-Type(0x2), S-Type(0x3), JRP-Type(0x7)
	unsigned int op = inst.is_01() ? inst.range(3, 0).to_uint() : 0xFF;
	uses_wide_sig.write(op == 0x2 || op == 0x3 || op == 0x7);
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_ex: ALU, 주소 ADDER, branch fail 감지
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_ex(void)
{
	// EX RS1/RS2 forwarding mux
	// 00=레지스터, 10=EX/MEM, 11=MEM/WB
	auto fwd8 = [&](unsigned int sel, sc_lv<8> reg_val) -> sc_lv<8> {
		if (sel == 0b10) return ex_mem_alu_out.read();
		if (sel == 0b11) return mem_wb_alu_out.read();
		return reg_val;
	};

	sc_lv<8> rs1 = fwd8(hde_ex_rs1_fwd_sel.read().to_uint(), id_ex_rs1.read());
	sc_lv<8> rs2 = fwd8(hde_ex_rs2_fwd_sel.read().to_uint(), id_ex_rs2.read());
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

	// Branch fail 감지: id_ex_opcode로 정확히 B-Type 판별
	bool b_type_in_ex = id_ex_valid.read()
	    && id_ex_opcode.read().is_01()
	    && id_ex_opcode.read().to_uint() == 0x4;

	bool fail = b_type_in_ex &&
	            (branch_taken.read() == SC_LOGIC_1) != id_ex_bpred.read();
	branch_pred_fail.write(fail);

	// correct PC 계산
	if (fail) {
		if (branch_taken.read() == SC_LOGIC_1) {
			// should taken: ID_EX.pc + sign_ext(imme_b)
			sc_int<16> off = sc_int<8>(id_ex_imme_b.read().to_uint());
			sc_uint<16> cpc = (sc_uint<16>)((sc_int<16>)(id_ex_pc.read().to_uint()) + off);
			branch_correct_pc.write(sc_lv<16>(cpc));
		} else {
			// should not-taken: ID_EX.pc + 3
			sc_uint<16> cpc = id_ex_pc.read().to_uint() + 3;
			branch_correct_pc.write(sc_lv<16>(cpc));
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// comb_mem: D-Cache 연결 (mem_read/mem_write 신호 드라이브)
// ─────────────────────────────────────────────────────────────────────────────

void thpg::wa824::Topboard::comb_mem(void)
{
	// ex_mem_ctrl에서 mem_read/write 비트 추출하여 D-Cache 포트 드라이브
	if (ex_mem_ctrl.read().is_01()) {
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
	// MEM/WB we4 신호
	if (mem_wb_ctrl.read().is_01()) {
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
	if (!mem_wb_valid.read()) {
		wb_data_l.write(sc_lv<8>(SC_LOGIC_0));
		wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
		return;
	}

	unsigned int wb_sel = ctrl_range(mem_wb_ctrl.read(), 13, 12);
	sc_lv<16> pc3 = mem_wb_pc3.read();

	switch (wb_sel) {
		case 0b00: // alu_out
			wb_data_l.write(mem_wb_alu_out.read());
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
		case 0b01: // PC+3 (JAL/JALR return addr, 16bit)
			wb_data_l.write(sc_lv<8>(pc3.range(7,  0)));
			wb_data_h.write(sc_lv<8>(pc3.range(15, 8)));
			break;
		case 0b10: // mem_out (Load)
			wb_data_l.write(mem_wb_mem_out.read());
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
		default:
			wb_data_l.write(sc_lv<8>(SC_LOGIC_0));
			wb_data_h.write(sc_lv<8>(SC_LOGIC_0));
			break;
	}
}
