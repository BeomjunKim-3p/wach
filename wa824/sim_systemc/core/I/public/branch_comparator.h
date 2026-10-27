#pragma once
#include <systemc.h>

// B-Type FUNCT [19:16]:
// 0x0 BEQ, 0x1 BNE, 0x2 BLT(signed), 0x3 BLTU(unsigned)

namespace thpg::wa824 {
	class BranchComparator : public sc_module {
	public:
		sc_in<sc_lv<8>>  rs1  {"RS1"};
		sc_in<sc_lv<8>>  rs2  {"RS2"};
		sc_in<sc_lv<4>>  funct{"FUNCT"};  // inst[19:16]
		sc_out<sc_logic> taken{"TAKEN"};

		BranchComparator(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
