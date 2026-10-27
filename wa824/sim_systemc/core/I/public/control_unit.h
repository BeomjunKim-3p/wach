#pragma once
#include <systemc.h>

// Control Unit — LUT
// 입력: inst[23:0] (opcode[7:4], funct[19:16] 사용)
// 출력: ctrl[13:0]
//   [0]    addr2_sel  : 0=inst[11:8](R-Type RS2),
//                       1=inst[3:0](S/B-Type RS2)
//   [1]    addr3_sel  : 0=inst[15:13](L/S wrs), 1=inst[19:17](JRP wrs)
//   [2]    alu_b_sel  : 0=R[RS2],             1=IMME
//   [3]    imme_sel   : 0=I-Type imme,        1=L&S-Type imme
//   [7:4]  alu_funct  : ALU funct 4bit
//   [8]    mem_read
//   [9]    mem_write
//   [10]   we4l
//   [11]   we4h
// invalid opcode/funct → mem_read/mem_write/we4l/we4h = 0

namespace thpg::wa824 {
	class ControlUnit : public sc_module {
	public:
		sc_in<sc_lv<24>>  inst{"INST"};
		sc_out<sc_lv<14>> ctrl{"CTRL"};

		ControlUnit(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
