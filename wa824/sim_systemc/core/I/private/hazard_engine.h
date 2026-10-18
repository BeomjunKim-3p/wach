#pragma once
#include <systemc.h>

// Hazard Detection Engine
//
// 처리 우선순위 (높은 순):
//   1. Reset                → 전체 flush
//   2. D-Cache miss         → 전체 stall
//   3. I-Cache miss         → PC+IF_ID stall, ID_EX flush
//   4. Branch fail / JALR   → IF_ID flush
//   5. Load-to-use          → PC+IF_ID stall, ID_EX flush (1사이클)
//   6. Pair hazard          → PC+IF_ID stall, ID_EX flush (1사이클)
//   7. Forwarding           → stall/flush 없음, sel 신호만 출력

namespace thpg::wa824 {
	class HazardEngine : public sc_module {
	public:
		// ── 입력 ──────────────────────────────────────────────
		sc_in<bool>      reset             {"RESET"};

		// 캐시 hit 여부
		sc_in<bool>      icache_hit        {"ICACHE_HIT"};
		sc_in<bool>      dcache_hit        {"DCACHE_HIT"};

		// 분기 관련
		sc_in<bool>      branch_pred_result{"BRANCH_PRED_RESULT"};
		sc_in<bool>      branch_pred_fail  {"BRANCH_PRED_FAIL"};
		sc_in<bool>      branch_result     {"BRANCH_RESULT"};  // EX 실제 결과

		// JALR
		sc_in<bool>      is_jalr           {"IS_JALR"};

		// IF 현재 명령어 (hazard 감지용 opcode/addr 파싱)
		sc_in<sc_lv<24>> inst              {"INST"};  // IF 단 inst

		// ID/EX 파이프라인 레지스터 내용
		sc_in<sc_lv<12>> id_ex_ctrl        {"ID_EX_CTRL"};   // ctrl[8]=mem_read, [10]=we4l, [11]=we4h
		sc_in<sc_lv<4>>  id_ex_rd          {"ID_EX_RD"};     // 쓰기 대상 레지스터 주소
		sc_in<sc_lv<3>>  id_ex_wrs         {"ID_EX_WRS"};    // wide read 주소 (페어)

		// IF/ID 파이프라인 레지스터 내용 (현재 ID 단에 있는 명령어 주소)
		sc_in<sc_lv<4>>  if_id_rs1         {"IF_ID_RS1"};    // inst[11:8]
		sc_in<sc_lv<4>>  if_id_rs2         {"IF_ID_RS2"};    // inst[15:12] or inst[7:4]
		sc_in<sc_lv<3>>  if_id_wrs         {"IF_ID_WRS"};    // wide read 주소
		sc_in<bool>      if_id_uses_wide   {"IF_ID_USES_WIDE"}; // L/S/JRP → wide read 사용

		// EX/MEM 파이프라인 레지스터 (WB forwarding 소스)
		sc_in<sc_lv<4>>  ex_mem_rd         {"EX_MEM_RD"};
		sc_in<bool>      ex_mem_we4l       {"EX_MEM_WE4L"};
		sc_in<bool>      ex_mem_we4h       {"EX_MEM_WE4H"};

		// MEM/WB 파이프라인 레지스터 (WB forwarding 소스)
		sc_in<sc_lv<4>>  mem_wb_rd         {"MEM_WB_RD"};
		sc_in<bool>      mem_wb_we4l       {"MEM_WB_WE4L"};
		sc_in<bool>      mem_wb_we4h       {"MEM_WB_WE4H"};

		// ── 출력 ──────────────────────────────────────────────
		// PC / 파이프라인 레지스터 제어
		sc_out<bool>     pc_stall          {"PC_STALL"};
		sc_out<bool>     if_id_stall       {"IF_ID_STALL"};
		sc_out<bool>     if_id_flush       {"IF_ID_FLUSH"};
		sc_out<bool>     id_ex_stall       {"ID_EX_STALL"};
		sc_out<bool>     id_ex_flush       {"ID_EX_FLUSH"};
		sc_out<bool>     ex_mem_stall      {"EX_MEM_STALL"};
		sc_out<bool>     ex_mem_flush      {"EX_MEM_FLUSH"};
		sc_out<bool>     mem_wb_stall      {"MEM_WB_STALL"};
		sc_out<bool>     mem_wb_flush      {"MEM_WB_FLUSH"};

		// Forwarding select
		sc_out<bool>     id_rs1_wb_fwd     {"ID_RS1_WB_FWD"};   // ID RS1 ← WB
		sc_out<bool>     id_rs2_wb_fwd     {"ID_RS2_WB_FWD"};   // ID RS2 ← WB
		sc_out<sc_lv<2>> ex_rs1_wb_fwd_sel {"EX_RS1_WB_FWD_SEL"}; // 00=reg, 10=EX/MEM, 11=MEM/WB
		sc_out<sc_lv<2>> ex_rs2_wb_fwd_sel {"EX_RS2_WB_FWD_SEL"};
		sc_out<sc_lv<2>> id_wrd_fwd_sel    {"ID_WRD_FWD_SEL"};  // wide read forwarding
		sc_out<sc_lv<2>> ex_wrd_fwd_sel    {"EX_WRD_FWD_SEL"};

		HazardEngine(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);

	private:
		// 각 hazard별 독립 감지 함수
		bool detect_icache_miss(void) const;
		bool detect_dcache_miss(void) const;
		bool detect_branch_fail(void) const;
		bool detect_jalr(void) const;
		bool detect_load_use(void) const;
		bool detect_pair_hazard(void) const;

		// Forwarding 계산
		void calc_forwarding(
			bool &id_rs1_fwd, bool &id_rs2_fwd,
			unsigned int &ex_rs1_sel, unsigned int &ex_rs2_sel,
			unsigned int &id_wrd_sel, unsigned int &ex_wrd_sel) const;
	};
}
