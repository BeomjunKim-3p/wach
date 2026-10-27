#pragma once
#include <systemc.h>

// Branch Prediction Engine — BTFNT (Backward Taken, Forward Not Taken)
// B-Type(opcode=0x4)이면 imm 부호비트(inst[23]) == 1 이거나 imm == 0x00 이면 taken
// 나머지 opcode → not-taken

namespace thpg::wa824 {
	class BPE : public sc_module {
	public:
		sc_in<sc_lv<24>> inst  {"INST"};
		sc_in<sc_lv<16>> pc   {"PC"};   // 다이어그램상 연결됨, 추후 활용

		sc_out<bool> branch_pred_result{"BRANCH_PRED_RESULT"};

		BPE(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
