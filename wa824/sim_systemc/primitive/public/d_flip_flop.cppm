module;
#include <systemc.h>

export module primitive.d_flip_flop;

export namespace thpg::primitive {
	class DFlipFlop : public sc_module {
	public:
		sc_in<sc_logic> d{"D"};
		sc_in<bool> clk{"CLK"};
		sc_out<sc_logic> q{"Q"};
		sc_out<sc_logic> q_bar{"Q_bar"};

		DFlipFlop(sc_module_name name);

		void refresh(void); 
	};
}


