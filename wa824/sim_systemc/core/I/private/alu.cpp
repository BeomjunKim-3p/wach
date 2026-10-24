#include <systemc.h>
#include "alu.h"

thpg::wa824::ALU::ALU(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b << funct << carry_in;

	if (tf) {
		std::string mn = std::string(this->name()) + "(ALU)";
		sc_trace(tf, a,        (mn + ".A").c_str());
		sc_trace(tf, b,        (mn + ".B").c_str());
		sc_trace(tf, funct,    (mn + ".FUNCT").c_str());
		sc_trace(tf, carry_in, (mn + ".CARRY_IN").c_str());
		sc_trace(tf, y,        (mn + ".Y").c_str());
		sc_trace(tf, flags,    (mn + ".FLAGS").c_str());
	}
}

void thpg::wa824::ALU::refresh(void)
{
	[&]{
		if (!a.read().is_01() || !b.read().is_01() ||
		    !funct.read().is_01() || !carry_in.read().is_01()) {
			y.write(sc_lv<8>(SC_LOGIC_X));
			flags.write(sc_lv<4>(SC_LOGIC_X));
			return;
		}

		sc_uint<9> va = a.read().to_uint();
		sc_uint<9> vb = b.read().to_uint();
		sc_int<8>  sa = sc_int<8>(a.read().to_uint());
		sc_int<8>  sb = sc_int<8>(b.read().to_uint());
		bool       ci = carry_in.read().to_bool();

		sc_uint<9> res9 = 0;
		sc_uint<8> res  = 0;
		bool       carry_out = false;
		bool       overflow  = false;

		switch (funct.read().to_uint()) {
			case 0x0: // ADD
				res9 = va + vb + (sc_uint<9>)ci;
				res  = res9.range(7, 0);
				carry_out = res9[8].to_bool();
				overflow  = (~(va ^ vb) & (va ^ res9) & 0x80) != 0;
				break;
			case 0x1: // SUB
				res9 = va - vb - (sc_uint<9>)ci;
				res  = res9.range(7, 0);
				carry_out = res9[8].to_bool();
				overflow  = ((va ^ vb) & (va ^ res9) & 0x80) != 0;
				break;
			case 0x2: // AND
				res = va & vb;
				break;
			case 0x3: // OR
				res = va | vb;
				break;
			case 0x4: // XOR
				res = va ^ vb;
				break;
			case 0x5: // SLL
				res = va << (vb & 0x7);
				break;
			case 0x6: // SRL
				res = va >> (vb & 0x7);
				break;
			case 0x7: // SRA
				res = (sc_uint<8>)((sc_int<8>)sa >> (vb & 0x7));
				break;
			case 0x8: // SLT (signed)
				res = (sa < sb) ? 1 : 0;
				break;
			case 0x9: // SLTU (unsigned)
				res = (va < vb) ? 1 : 0;
				break;
			default:
				y.write(sc_lv<8>(SC_LOGIC_X));
				flags.write(sc_lv<4>(SC_LOGIC_X));
				return;
		}

		y.write(sc_lv<8>(res));

		sc_lv<4> f;
		f[3] = res[7] ? SC_LOGIC_1 : SC_LOGIC_0; // N
		f[2] = (res == 0)              ? SC_LOGIC_1 : SC_LOGIC_0; // Z
		f[1] = carry_out               ? SC_LOGIC_1 : SC_LOGIC_0; // C
		f[0] = overflow                ? SC_LOGIC_1 : SC_LOGIC_0; // V
		flags.write(f);
	}();
}
