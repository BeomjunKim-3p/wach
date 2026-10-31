#ifndef TOPBOARD
#define TOPBOARD
#include <systemc.h>

#include <cstdint>
#include <string>
#include <vector>

#include "alu.h"
#include "branch_comparator.h"
#include "branch_pred_engine.h"
#include "control_unit.h"
#include "dcache.h"
#include "flow_controller.h"
#include "hazard_engine.h"
#include "icache.h"
#include "imm_gen.h"
#include "mem_arbiter.h"
#include "register_file_redge.h"

// ctrl[13:0] 비트 정의
//   [0]    addr2_sel
//   [1]    addr3_sel
//   [2]    alu_b_sel
//   [3]    imme_sel
//   [7:4]  alu_funct
//   [8]    mem_read
//   [9]    mem_write
//   [10]   we4l
//   [11]   we4h
//   [13:12] wb_sel  (00=alu_out, 01=PC+3, 10=mem_out)

namespace thpg::wa824 {

// 디버그 스냅샷: 모듈/파이프라인 레지스터의 값을 이름-값 쌍으로 보관한다.
struct DebugValue {
	std::string name;
	std::string value;
};

struct DebugGroup {
	std::string name;
	std::vector<DebugValue> values;
};

struct DebugSnapshot {
	uint64_t cycle{0};
	std::vector<DebugGroup> groups;
};

struct DebugCollector;

class Topboard : public sc_module {
public:
	// ── 외부 포트 ────────────────────────────────────────────
	sc_in<bool>       clk  {"CLK"};
	sc_in<bool>       reset{"RESET"};

	// ── 외부에서 설정하는 MMIO 영역/정책 ────────────────────
	// CSR 없이 testbench/플랫폼 컨트롤러가 직접 설정한다.
	//   is_mmio = enable && ((addr & mask) == (base & mask))
	sc_in<bool>       mmio_enable  {"MMIO_ENABLE"};
	sc_in<sc_lv<16>>  mmio_base    {"MMIO_BASE"};
	sc_in<sc_lv<16>>  mmio_mask    {"MMIO_MASK"};
	sc_in<bool>       mmio_ordered {"MMIO_ORDERED"};

	// ── 디버그용 캐시 무효화 핀 ──────────────────────────────
	// 테스트벤치가 메인 메모리를 수정한 뒤 해당 라인 또는 전체 캐시를
	// 무효화할 때 사용한다. 실제 코어 동작에는 관여하지 않는다.
	sc_in<bool>       dbg_ic_invalidate     {"DBG_IC_INVALIDATE"};
	sc_in<sc_lv<16>>  dbg_ic_invalidate_addr{"DBG_IC_INVALIDATE_ADDR"};
	sc_in<bool>       dbg_ic_invalidate_all {"DBG_IC_INVALIDATE_ALL"};
	sc_in<bool>       dbg_dc_invalidate     {"DBG_DC_INVALIDATE"};
	sc_in<sc_lv<16>>  dbg_dc_invalidate_addr{"DBG_DC_INVALIDATE_ADDR"};
	sc_in<bool>       dbg_dc_invalidate_all {"DBG_DC_INVALIDATE_ALL"};

	// 메인 메모리 버스 (MemArbiter 경유)
	sc_out<sc_lv<16>> mem_addr {"MEM_ADDR"};
	sc_out<sc_lv<8>>  mem_wdata{"MEM_WDATA"};
	sc_out<bool>      mem_we   {"MEM_WE"};
	sc_out<bool>      mem_re   {"MEM_RE"};
	sc_in<sc_lv<8>>   mem_rdata{"MEM_RDATA"};
	sc_in<bool>       mem_ready{"MEM_READY"};

	Topboard(sc_module_name name, sc_trace_file *tf = nullptr);

	// 디버거용 전체 상태 스냅샷
	DebugSnapshot capture_debug(void);

	// 디버거용 메모리 peek. D-Cache의 pending write buffer와 cache line을
	// 먼저 확인하고, 없으면 false를 돌려준다. 시뮬레이션 시간은 진행하지 않는다.
	bool debug_read_byte(uint16_t address, uint8_t &value) const;

	// ── tick: 파이프라인 레지스터 (clk.pos()) ────────────────
	void tick_debug(void);
	void tick_pc(void);
	void tick_if_id(void);
	void tick_id_ex(void);
	void tick_ex_mem(void);
	void tick_mem_wb(void);

	// ── comb: 조합논리 ────────────────────────────────────────
	void comb_if(void);
	void comb_id(void);
	void comb_ex(void);
	void comb_mem(void);
	void comb_wb(void);

private:
	friend struct DebugCollector;

	void debug_invalidate(void);

	uint64_t m_cycle{0};

	// ── 서브모듈 ─────────────────────────────────────────────
	ICache            icache      {"ICACHE"};
	DCache            dcache      {"DCACHE"};
	BPE               bpe         {"BPE"};
	FlowController    fc          {"FC"};
	ControlUnit       cu          {"CU"};
	ImmGen            immgen      {"IMMGEN"};
	RegisterFileREdge regfile     {"REGFILE"};
	BranchComparator  branch_cmp  {"BRANCH_CMP"};
	ALU               alu         {"ALU"};
	HazardEngine      hde         {"HDE"};
	MemArbiter        mem_arbiter {"MEM_ARBITER"};

	// ── PC 레지스터 ──────────────────────────────────────────
	sc_signal<sc_lv<16>> pc_reg{"pc_reg"};

	// ── IF_ID 파이프라인 레지스터 ────────────────────────────
	sc_signal<bool>      if_id_valid{"if_id_valid"};
	sc_signal<sc_lv<16>> if_id_pc   {"if_id_pc"};
	sc_signal<sc_lv<16>> if_id_pc3  {"if_id_pc3"};
	sc_signal<sc_lv<24>> if_id_inst {"if_id_inst"};

	// ── ID_EX 파이프라인 레지스터 ────────────────────────────
	sc_signal<bool>      id_ex_valid   {"id_ex_valid"};
	sc_signal<sc_lv<16>> id_ex_pc      {"id_ex_pc"};
	sc_signal<sc_lv<16>> id_ex_pc3     {"id_ex_pc3"};
	sc_signal<sc_lv<14>> id_ex_ctrl    {"id_ex_ctrl"};
	sc_signal<sc_lv<4>>  id_ex_rd_sel  {"id_ex_rd_sel"};
	sc_signal<sc_lv<4>>  id_ex_rs1_addr{"id_ex_rs1_addr"};
	sc_signal<sc_lv<4>>  id_ex_rs2_addr{"id_ex_rs2_addr"};
	sc_signal<sc_lv<3>>  id_ex_wrs_addr{"id_ex_wrs_addr"};
	sc_signal<sc_lv<8>>  id_ex_rs1     {"id_ex_rs1"};
	sc_signal<sc_lv<8>>  id_ex_rs2     {"id_ex_rs2"};
	sc_signal<sc_lv<8>>  id_ex_rs_store{"id_ex_rs_store"}; // S-Type store data
	sc_signal<sc_lv<16>> id_ex_wrs_data{"id_ex_wrs_data"}; // R[WRS]
	sc_signal<sc_lv<12>> id_ex_imme    {"id_ex_imme"};  // L/S-Type 12bit
	sc_signal<sc_lv<4>>  id_ex_funct   {"id_ex_funct"};    // inst[19:16]

	// ── EX_MEM 파이프라인 레지스터 ───────────────────────────
	sc_signal<bool>      ex_mem_valid   {"ex_mem_valid"};
	sc_signal<sc_lv<16>> ex_mem_pc      {"ex_mem_pc"};
	sc_signal<sc_lv<16>> ex_mem_pc3     {"ex_mem_pc3"};
	sc_signal<sc_lv<14>> ex_mem_ctrl    {"ex_mem_ctrl"};
	sc_signal<sc_lv<4>>  ex_mem_rd_sel  {"ex_mem_rd_sel"};
	sc_signal<sc_lv<8>>  ex_mem_alu_out {"ex_mem_alu_out"};
	sc_signal<sc_lv<16>> ex_mem_addr    {"ex_mem_addr"};
	sc_signal<sc_lv<8>>  ex_mem_rs_store{"ex_mem_rs_store"};

	// ── MEM_WB 파이프라인 레지스터 ───────────────────────────
	sc_signal<bool>      mem_wb_valid   {"mem_wb_valid"};
	sc_signal<sc_lv<16>> mem_wb_pc      {"mem_wb_pc"};
	sc_signal<sc_lv<16>> mem_wb_pc3     {"mem_wb_pc3"};
	sc_signal<sc_lv<14>> mem_wb_ctrl    {"mem_wb_ctrl"};
	sc_signal<sc_lv<4>>  mem_wb_rd_sel  {"mem_wb_rd_sel"};
	sc_signal<sc_lv<8>>  mem_wb_alu_out {"mem_wb_alu_out"};
	sc_signal<sc_lv<8>>  mem_wb_mem_out {"mem_wb_mem_out"};
	sc_signal<sc_lv<16>> mem_wb_pc3_val {"mem_wb_pc3_val"};

	// ── 조합논리 내부 신호 ────────────────────────────────────
	// IF
	sc_signal<sc_lv<24>> if_inst_sig       {"if_inst_sig"}; // ICache → BPE/HDE/IF latch
	sc_signal<bool>      ic_hit_sig        {"ic_hit_sig"};
	sc_signal<sc_lv<16>> next_pc           {"next_pc"};
	sc_signal<sc_lv<2>>  fc_jump_type      {"fc_jump_type"};
	sc_signal<sc_lv<8>>  fc_b_offset       {"fc_b_offset"}; // IF BPE 예측 target용
	sc_signal<sc_lv<16>> fc_jr_imme    {"fc_jr_imme"};
	sc_signal<sc_lv<16>> fc_ja_imme    {"fc_ja_imme"};
	sc_signal<bool>      bpe_result        {"bpe_result"};
	sc_signal<bool>      not_taken_restore {"not_taken_restore"}; // 예측=taken, 실제=not-taken
	sc_signal<bool>      taken_restore     {"taken_restore"};     // 예측=not-taken, 실제=taken

	// ID
	sc_signal<sc_lv<14>> cu_ctrl          {"cu_ctrl"};
	sc_signal<sc_lv<8>>  immgen_i         {"immgen_i"};
	sc_signal<sc_lv<8>>  immgen_b         {"immgen_b"}; // ID branch restore용
	sc_signal<sc_lv<12>> immgen_ls        {"immgen_ls"};
	sc_signal<sc_lv<16>> immgen_ja        {"immgen_ja"};
	sc_signal<sc_lv<12>> immgen_jrp       {"immgen_jrp"};
	// RegFile 출력 (forwarding mux 이전)
	sc_signal<sc_lv<8>>  rf_rs1_raw       {"rf_rs1_raw"};
	sc_signal<sc_lv<8>>  rf_rs2_raw       {"rf_rs2_raw"};
	sc_signal<sc_lv<16>> rf_wrs_raw       {"rf_wrs_raw"};
	// Forwarding 후 실제 사용값
	sc_signal<sc_lv<8>>  id_rs1_data      {"id_rs1_data"};
	sc_signal<sc_lv<8>>  id_rs2_data      {"id_rs2_data"};
	sc_signal<sc_lv<16>> id_wrs_data      {"id_wrs_data"};
	sc_signal<sc_lv<12>> id_imme_sel      {"id_imme_sel"};  // imme mux 출력 (최대 12bit)
	// WB writeback data (forwarding 소스)
	sc_signal<sc_lv<8>>  wb_data_l        {"wb_data_l"};
	sc_signal<sc_lv<8>>  wb_data_h        {"wb_data_h"};
	sc_signal<bool>      wb_we4l          {"wb_we4l"};
	sc_signal<bool>      wb_we4h          {"wb_we4h"};
	// Branch comparator (ID단)
	sc_signal<sc_lv<4>>  id_branch_funct  {"id_branch_funct"}; // if_id_inst[19:16]
	sc_signal<sc_logic>  branch_taken     {"branch_taken"};

	// EX
	sc_signal<sc_lv<8>>  ex_rs1_data      {"ex_rs1_data"};  // forwarding 후
	sc_signal<sc_lv<8>>  ex_rs2_data      {"ex_rs2_data"};
	sc_signal<sc_lv<8>>  ex_alu_b         {"ex_alu_b"};     // ALU B mux 출력
	sc_signal<sc_lv<8>>  alu_out_sig      {"alu_out_sig"};
	sc_signal<sc_lv<4>>  alu_flags        {"alu_flags"};
	sc_signal<sc_lv<16>> ex_addr_out      {"ex_addr_out"};  // WRS+IMME 주소 ADDER

	// HDE 출력
	sc_signal<bool>      hde_pc_stall        {"hde_pc_stall"};
	sc_signal<bool>      hde_if_id_stall     {"hde_if_id_stall"};
	sc_signal<bool>      hde_if_id_flush     {"hde_if_id_flush"};
	sc_signal<bool>      hde_id_ex_stall     {"hde_id_ex_stall"};
	sc_signal<bool>      hde_id_ex_flush     {"hde_id_ex_flush"};
	sc_signal<bool>      hde_ex_mem_stall    {"hde_ex_mem_stall"};
	sc_signal<bool>      hde_ex_mem_flush    {"hde_ex_mem_flush"};
	sc_signal<bool>      hde_mem_wb_stall    {"hde_mem_wb_stall"};
	sc_signal<bool>      hde_mem_wb_flush    {"hde_mem_wb_flush"};
	sc_signal<bool>      hde_id_rs1_wb_fwd  {"hde_id_rs1_wb_fwd"};
	sc_signal<bool>      hde_id_rs2_wb_fwd  {"hde_id_rs2_wb_fwd"};
	sc_signal<sc_lv<2>>  hde_ex_rs1_fwd_sel {"hde_ex_rs1_fwd_sel"};
	sc_signal<sc_lv<2>>  hde_ex_rs2_fwd_sel {"hde_ex_rs2_fwd_sel"};
	sc_signal<sc_lv<2>>  hde_id_wrd_fwd_sel {"hde_id_wrd_fwd_sel"};
	sc_signal<sc_lv<2>>  hde_ex_wrd_fwd_sel {"hde_ex_wrd_fwd_sel"};
	sc_signal<bool>      hde_ex_mem_we4l    {"hde_ex_mem_we4l"};
	sc_signal<bool>      hde_ex_mem_we4h    {"hde_ex_mem_we4h"};
	sc_signal<bool>      hde_mem_wb_we4l    {"hde_mem_wb_we4l"};
	sc_signal<bool>      hde_mem_wb_we4h    {"hde_mem_wb_we4h"};

	// MemArbiter ↔ ICache/DCache
	sc_signal<sc_lv<16>> arb_ic_addr  {"arb_ic_addr"};
	sc_signal<bool>      arb_ic_re    {"arb_ic_re"};
	sc_signal<sc_lv<8>>  arb_ic_rdata {"arb_ic_rdata"};
	sc_signal<bool>      arb_ic_ready {"arb_ic_ready"};
	sc_signal<bool>      arb_ic_grant {"arb_ic_grant"};
	sc_signal<sc_lv<16>> arb_dc_addr  {"arb_dc_addr"};
	sc_signal<sc_lv<8>>  arb_dc_wdata {"arb_dc_wdata"};
	sc_signal<bool>      arb_dc_we    {"arb_dc_we"};
	sc_signal<bool>      arb_dc_re    {"arb_dc_re"};
	sc_signal<bool>      arb_dc_urgent{"arb_dc_urgent"};
	sc_signal<sc_lv<8>>  arb_dc_rdata {"arb_dc_rdata"};
	sc_signal<bool>      arb_dc_ready {"arb_dc_ready"};
	sc_signal<bool>      arb_dc_grant {"arb_dc_grant"};

	// RegFile addr mux
	sc_signal<sc_lv<4>>  rf_addr2_mux {"rf_addr2_mux"};
	sc_signal<sc_lv<3>>  rf_addr3_mux {"rf_addr3_mux"};

	// is_jalr (ID 조합신호: IF/ID의 JRP/JALR)
	sc_signal<bool>      is_jalr_sig   {"is_jalr_sig"};

	// ALU carry_in (현재 항상 0)
	sc_signal<sc_logic>  alu_carry_in  {"alu_carry_in"};

	// D-Cache mem_read/write 중간 신호 (ctrl에서 파생)
	sc_signal<bool>      dc_mem_read   {"dc_mem_read"};
	sc_signal<bool>      dc_mem_write  {"dc_mem_write"};
	sc_signal<bool>      dc_hit_sig    {"dc_hit_sig"};
	sc_signal<sc_lv<8>>  dc_rdata_sig  {"dc_rdata_sig"};

	// 6번: rs1_addr 전용 신호
	sc_signal<sc_lv<4>>  rf_addr1      {"rf_addr1"};



	// 8번: id_ex_opcode (B-Type 정확 판별용)
	sc_signal<sc_lv<4>>  id_ex_opcode  {"id_ex_opcode"};
};

} // namespace thpg::wa824

#endif 
