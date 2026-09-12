#include "alu.h"
#include <systemc.h>

namespace thpg::core::i {
	ALU::ALU(sc_module_name name, sc_trace_file *tf)
		: sc_module(name),
		andgate{"AndGate", tf},
		orgate{"OrGate", tf},
		xorgate{"XorGate", tf},
		adder{"Adder", tf},
		subtractor{"Subtractor", tf},
		shifter{"Shifter", tf},
		comparator{"Comparator", tf},
		outmux{"outmux"}
	{
		SC_METHOD(refresh);
		sensitive << a << b << carry << is_rotate << direction_right << inst_in;

		SC_METHOD(cmpref);
		sensitive << less << equal << greater;

		SC_METHOD(invref);
		sensitive << invert << b;

		andgate.a(a);
		andgate.b(b);
		andgate.out(andout);

		orgate.a(a);
		orgate.b(b);
		orgate.out(orout);

		xorgate.a(a);
		xorgate.b(b);
		xorgate.out(xorout);

		adder.a(a);
		//adder.b(b);
		adder.b(invb);
		adder.carry_in(carry);
		adder.out(adderout);
		adder.carry_out(addercarryout);

		subtractor.a(a);
		subtractor.b(b);
		subtractor.borrow_in(carry);
		subtractor.out(subtractorout);
		subtractor.borrow_out(subtractorcarryout);

		shifter.a(a);
		shifter.b(b);
		shifter.is_rotate(is_rotate);
		shifter.direction_right(direction_right);
		shifter.out(shifterout);

		comparator.a(a);
		comparator.b(b);
		comparator.less(less);
		comparator.equal(equal);
		comparator.greater(greater);

		outmux.in[0](andout);
		outmux.in[1](orout);
		outmux.in[2](xorout);
		outmux.in[3](adderout);
		outmux.in[4](subtractorout);
		outmux.in[5](shifterout);
		outmux.in[6](cmpout);
		outmux.out(muxout);
		outmux.sel(muxsel);

		
		if (tf) {
			sc_trace(tf, a, (std::string(this->name()) + "(alu)" + ".a").c_str());
			sc_trace(tf, b, (std::string(this->name()) + "(alu)" + ".b").c_str());
			sc_trace(tf, carry, (std::string(this->name()) + "(alu)" + ".carry").c_str());
			sc_trace(tf, is_rotate, (std::string(this->name()) + "(alu)" + ".is_rotate").c_str());
			sc_trace(tf, direction_right, (std::string(this->name()) + "(alu)" + ".direction_right").c_str());
			sc_trace(tf, inst_in, (std::string(this->name()) + "(alu)" + ".inst_in").c_str());
			sc_trace(tf, y, (std::string(this->name()) + "(alu)" + ".y").c_str());
			sc_trace(tf, flag_out, (std::string(this->name()) + "(alu)" + ".flag_out").c_str());
			sc_trace(tf, andout, (std::string(this->name()) + "(alu)" + ".andout").c_str());
			sc_trace(tf, orout, (std::string(this->name()) + "(alu)" + ".orout").c_str());
			sc_trace(tf, xorout, (std::string(this->name()) + "(alu)" + ".xorout").c_str());
			sc_trace(tf, carryout, (std::string(this->name()) + "(alu)" + ".carryout").c_str());
			sc_trace(tf, invb, (std::string(this->name()) + "(alu)" + ".invb").c_str());
			sc_trace(tf, adderout, (std::string(this->name()) + "(alu)" + ".adderout").c_str());
			sc_trace(tf, addercarryout, (std::string(this->name()) + "(alu)" + ".addercarryout").c_str());
			sc_trace(tf, subtractorcarryout, (std::string(this->name()) + "(alu)" + ".subtractorcarryout").c_str());
			sc_trace(tf, shifterout, (std::string(this->name()) + "(alu)" + ".shifterout").c_str());
			sc_trace(tf, less, (std::string(this->name()) + "(alu)" + ".less").c_str());
			sc_trace(tf, equal, (std::string(this->name()) + "(alu)" + ".equal").c_str());
			sc_trace(tf, greater, (std::string(this->name()) + "(alu)" + ".greater").c_str());
			sc_trace(tf, muxout, (std::string(this->name()) + "(alu)" + ".muxout").c_str());
			sc_trace(tf, muxsel, (std::string(this->name()) + "(alu)" + ".muxsel").c_str());
		}
	}

	void ALU::cmpref(void)
	{
		sc_lv<8> temp(8);
		temp[0] = less.read();
		temp[1] = equal.read();
		temp[2] = greater.read();
		temp[3] = SC_LOGIC_0;
		temp[4] = SC_LOGIC_0;
		temp[5] = SC_LOGIC_0;
		temp[6] = SC_LOGIC_0;
		temp[7] = SC_LOGIC_0;
		cmpout.write(temp);
	}

	void ALU::invref(void)
	{
		if (invert.read() == SC_LOGIC_1) {
			invb = ~b.read();
		}else {
			invb = b.read();
		}
	}

	void ALU::refresh(void)
	{
		[&]{
			if (!(a.read().is_01() && b.read().is_01() && carry.read().is_01() && inst_in.read().is_01() && invert.read().is_01())) {
				y.write(sc_lv<8>(SC_LOGIC_X));
				return;
			}
			y.write(muxout);

			switch (inst_in.read().to_uint()) {
				case 0:
					//add
					muxsel.write(3);
					break;

				case 1:
					//and
					muxsel.write(0);
					break;

				case 2:
					//or
					muxsel.write(1);
					break;

				case 3:
					//xor
					muxsel.write(2);
					break;

				case 4:
					//shift
					muxsel.write(5);
					break;

				case 5:
					//cmp
					muxsel.write(6);
					break;
			}
		}();
	}
}



