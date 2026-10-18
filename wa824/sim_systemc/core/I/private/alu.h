#pragma once
#include <systemc.h>

// R-Type FUNCT [23:20]:
// 0x0 ADD, 0x1 SUB, 0x2 AND, 0x3 OR, 0x4 XOR
// 0x5 SLL, 0x6 SRL, 0x7 SRA, 0x8 SLT, 0x9 SLTU
// I-Type FUNCT 동일 (SUB 없음)

namespace thpg::wa824 {
	class ALU : public sc_module {
	public:
		sc_in<sc_lv<8>>  a       {"A"};
		sc_in<sc_lv<8>>  b       {"B"};
		sc_in<sc_lv<4>>  funct   {"FUNCT"};   // inst[23:20]
		sc_in<sc_logic>  carry_in{"CARRY_IN"}; // 외부 carry (ADC 등)
		sc_out<sc_lv<8>> y       {"Y"};
		sc_out<sc_lv<4>> flags   {"FLAGS"};    // [3]=N [2]=Z [1]=C [0]=V

		ALU(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
