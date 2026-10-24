#pragma once
#include <systemc.h>

// Hazard Detection Engine
//
// 처리 우선순위 (높은 순):
//   1. Reset                → 전체 flush
//   2. D-Cache miss         → 전체 stall
//   3. I-Cache miss         → PC+IF_ID stall, ID_EX flush
//   4. Branch restore       → IF_ID flush + restore 신호
//   5. JRP/JALR             → IF_ID flush
//   6. Branch source hazard → PC+IF_ID stall, ID_EX flush (WB 도착까지)
//   7. Load-to-use          → PC+IF_ID stall, ID_EX flush (1사이클)
//   8. Pair hazard          → PC+IF_ID stall, ID_EX flush (1사이클)
//   9. Forwarding           → stall/flush 없음, sel 신호만 출력
//
// inst 포트: IF/ID 단 현재 명령어
// m_prev_inst: tick()에서 매 사이클 갱신, ID단 명령어 (= 직전 IF/ID)

namespace thpg::wa824 {
	class HazardEngine : public sc_module {
	public:
		// ── 입력 ──────────────────────────────────────────────
		sc_in<bool>      clk              {"CLK"};
		sc_in<bool>      reset            {"RESET"};

		// 캐시 hit 여부
		sc_in<bool>      icache_hit       {"ICACHE_HIT"};
		sc_in<bool>      dcache_hit       {"DCACHE_HIT"};

		// 분기 관련
		sc_in<bool>      branch_pred_result{"BRANCH_PRED_RESULT"}; // IF단 BPE 예측값
		sc_in<sc_logic>  branch_result    {"BRANCH_RESULT"};       // ID단 BranchComparator 실제 결과

		// JRP/JALR
		sc_in<bool>      is_jalr          {"IS_JALR"};

		// IF/ID 단 현재 명령어 (hazard 감지용 opcode/addr 파싱)
		sc_in<sc_lv<24>> inst             {"INST"};

		// IF 단 현재 명령어/PC (BPE 예측 캡처용)
		sc_in<sc_lv<24>> if_inst          {"IF_INST"};
		sc_in<sc_lv<16>> if_pc            {"IF_PC"};

		// ID/EX 파이프라인 레지스터 내용
		sc_in<sc_lv<14>> id_ex_ctrl       {"ID_EX_CTRL"};  // ctrl[8]=mem_read, [10]=we4l, [11]=we4h
		sc_in<sc_lv<4>>  id_ex_rd         {"ID_EX_RD"};    // 쓰기 대상 레지스터 주소
		sc_in<sc_lv<4>>  id_ex_rs1_addr   {"ID_EX_RS1_ADDR"}; // EX단 소스1 주소
		sc_in<sc_lv<4>>  id_ex_rs2_addr   {"ID_EX_RS2_ADDR"}; // EX단 소스2 주소
		sc_in<sc_lv<4>>  id_ex_opcode     {"ID_EX_OPCODE"};
		sc_in<sc_lv<3>>  id_ex_wrs        {"ID_EX_WRS"};   // wide read 주소 (페어)

		// EX/MEM 파이프라인 레지스터 (WB forwarding / D-Cache hazard 소스)
		sc_in<sc_lv<14>> ex_mem_ctrl      {"EX_MEM_CTRL"};
		sc_in<sc_lv<4>>  ex_mem_rd        {"EX_MEM_RD"};
		sc_in<bool>      ex_mem_we4l      {"EX_MEM_WE4L"};
		sc_in<bool>      ex_mem_we4h      {"EX_MEM_WE4H"};

		// MEM/WB 파이프라인 레지스터 (WB forwarding 소스)
		sc_in<sc_lv<4>>  mem_wb_rd        {"MEM_WB_RD"};
		sc_in<bool>      mem_wb_we4l      {"MEM_WB_WE4L"};
		sc_in<bool>      mem_wb_we4h      {"MEM_WB_WE4H"};

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

		// Branch prediction 복구 신호
		sc_out<bool>     not_taken_restore {"NOT_TAKEN_RESTORE"}; // 예측=taken, 실제=not-taken
		sc_out<bool>     taken_restore     {"TAKEN_RESTORE"};     // 예측=not-taken, 실제=taken

		// Forwarding select
		sc_out<bool>     id_rs1_wb_fwd     {"ID_RS1_WB_FWD"};
		sc_out<bool>     id_rs2_wb_fwd     {"ID_RS2_WB_FWD"};
		sc_out<sc_lv<2>> ex_rs1_wb_fwd_sel {"EX_RS1_WB_FWD_SEL"};
		sc_out<sc_lv<2>> ex_rs2_wb_fwd_sel {"EX_RS2_WB_FWD_SEL"};
		sc_out<sc_lv<2>> id_wrd_fwd_sel    {"ID_WRD_FWD_SEL"};
		sc_out<sc_lv<2>> ex_wrd_fwd_sel    {"EX_WRD_FWD_SEL"};

		HazardEngine(sc_module_name name, sc_trace_file *tf = nullptr);

		void tick(void);    // clk.pos(): 예측 캡처/소비, m_prev_inst 갱신
		void refresh(void); // 조합논리: hazard 감지 및 신호 출력

		// 디버그용 HDE 내부 예측 상태
		bool       pred_valid(void) const { return m_pred_valid; }
		bool       pred_taken(void) const { return m_pred_taken; }
		sc_lv<16>  pred_pc(void)    const { return m_pred_pc; }

	private:
		// ID단 명령어 추적 (tick에서 inst를 1사이클 지연)
		sc_lv<24> m_prev_inst{SC_LOGIC_0};

		// IF에서 캡처한 BPE 예측. ID단에서 실제 branch 결과와 비교한다.
		bool      m_pred_valid{false};
		bool      m_pred_taken{false};
		sc_lv<16> m_pred_pc{SC_LOGIC_0};

		// 각 hazard별 독립 감지 함수
		bool detect_icache_miss(void) const;
		bool detect_dcache_miss(void) const;
		bool detect_not_taken_restore(void) const;
		bool detect_taken_restore(void) const;
		bool detect_jalr(void) const;
		bool detect_branch_source_hazard(void) const;
		bool detect_load_use(void) const;
		bool detect_pair_hazard(void) const;

		// Forwarding 계산
		void calc_forwarding(
			bool &id_rs1_fwd, bool &id_rs2_fwd,
			unsigned int &ex_rs1_sel, unsigned int &ex_rs2_sel,
			unsigned int &id_wrd_sel, unsigned int &ex_wrd_sel) const;
	};
}
