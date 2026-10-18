#pragma once
#include <systemc.h>

// Flow Controller
// SC_METHOD 1: refresh_imme()      — 오프셋 추출
// SC_METHOD 2: refresh_jump_type() — Jump Type 결정
//
// jump_type[1:0]:
//   00 = branch or normal
//        (branch_pred=1이면 B-Offset 선택, 0이면 IMME:0x0003)
//   01 = JR-Type  (PC + JR imme, relative)
//   10 = JRP/JALR (R[WRS] + JRP imme)
//   11 = JA-Type  (0x0000 + JA imme, absolute)
//
// JALR(is_jalr=1)은 jump_type=10으로 처리 (JRP와 동일 경로)
// JRP imme는 ID 단 ImmGen에서 공급

namespace thpg::wa824 {
	class FlowController : public sc_module {
	public:
		sc_in<sc_lv<24>> inst       {"INST"};
		sc_in<bool>      branch_pred{"BRANCH_PRED"};
		sc_in<bool>      is_jalr    {"IS_JALR"};

		// 오프셋 출력 (PC ADDER 입력 mux로)
		sc_out<sc_lv<8>>  b_offset  {"B_OFFSET"};   // B-Type:  {inst[19:16], inst[15:12]}
		sc_out<sc_lv<16>> jr_imme   {"JR_IMME"};    // JR-Type: {inst[23:16], inst[15:8]}
		sc_out<sc_lv<16>> ja_imme   {"JA_IMME"};    // JA-Type: {inst[23:16], inst[15:8]} (동일 비트, 별도 출력)

		sc_out<sc_lv<2>> jump_type  {"JUMP_TYPE"};

		FlowController(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh_imme(void);
		void refresh_jump_type(void);
	};
}
