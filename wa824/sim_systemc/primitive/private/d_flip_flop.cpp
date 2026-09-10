module;
#include <systemc.h>
#include <unistd.h>

module primitive.d_flip_flop;

namespace thpg::primitive {
	DFlipFlop::DFlipFlop(sc_module_name name, sc_trace_file *tf) : sc_module(name)
	{
		SC_METHOD(refresh);
		sensitive << clk.pos();
	
		if (tf) {
			sc_trace(tf, d, (std::string(this->name()) + "(shifter)" + ".b").c_str());
			sc_trace(tf, clk, (std::string(this->name()) + "(shifter)" + ".is_rotate").c_str());
			sc_trace(tf, q, (std::string(this->name()) + "(shifter)" + ".direction_right").c_str());
			sc_trace(tf, q_bar, (std::string(this->name()) + "(shifter)" + ".out").c_str());
		}
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


