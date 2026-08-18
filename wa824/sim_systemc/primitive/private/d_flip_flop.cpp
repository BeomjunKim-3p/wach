module;
#include <systemc.h>
#include <unistd.h>

module primitive.d_flip_flop;

namespace thpg::primitive {
	DFlipFlop::DFlipFlop(sc_module_name name) : sc_module(name) {
		SC_METHOD(refresh);
		sensitive << clk.pos();
	}


	void DFlipFlop::refresh(void)
	{
		if (d.read().is_01()) {
			q.write(d.read());
			q_bar.write(~d.read());
		} else {
			q.write(sc_logic_X);
			q_bar.write(sc_logic_X);
		}
	}
}


