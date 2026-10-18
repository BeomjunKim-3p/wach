#include <systemc.h>
#include "branch_comparator.h"

thpg::wa824::BranchComparator::BranchComparator(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << rs1 << rs2 << funct;

	if (tf) {
		std::string mn = std::string(this->name()) + "(BranchComparator)";
		sc_trace(tf, rs1,   (mn + ".RS1").c_str());
		sc_trace(tf, rs2,   (mn + ".RS2").c_str());
		sc_trace(tf, funct, (mn + ".FUNCT").c_str());
		sc_trace(tf, taken, (mn + ".TAKEN").c_str());
	}
}

void thpg::wa824::BranchComparator::refresh(void)
{
	[&]{
		if (!rs1.read().is_01() || !rs2.read().is_01() || !funct.read().is_01()) {
			taken.write(SC_LOGIC_X);
			return;
		}

		sc_uint<8> a = rs1.read().to_uint();
		sc_uint<8> b = rs2.read().to_uint();
		sc_int<8>  sa = sc_int<8>(rs1.read().to_uint());
		sc_int<8>  sb = sc_int<8>(rs2.read().to_uint());

		bool result = false;
		switch (funct.read().to_uint()) {
			case 0x0: result = (a == b);  break;  // BEQ
			case 0x1: result = (a != b);  break;  // BNE
			case 0x2: result = (sa < sb); break;  // BLT  (signed)
			case 0x3: result = (a  < b);  break;  // BLTU (unsigned)
			default:  taken.write(SC_LOGIC_X); return;
		}

		taken.write(result ? SC_LOGIC_1 : SC_LOGIC_0);
	}();
}
