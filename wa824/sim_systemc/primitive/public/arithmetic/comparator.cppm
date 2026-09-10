module;
#include <systemc.h>

export module primitive.arithmetic.comparator;

import primitive.arithmetic.subtractor;


export namespace thpg::primitive::arithmetic {
	template <int N = 8>
	class Comparator : public sc_module {
	public:
		sc_in<sc_lv<N>> a{"A"};
		sc_in<sc_lv<N>> b{"B"};
		sc_out<sc_logic> less{"Less"};
		sc_out<sc_logic> equal{"Equal"};
		sc_out<sc_logic> greater{"Greater"};
		

		Comparator(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	
	private:
		Subtractor<N> subtractor{"Subtractor"};
		sc_signal<sc_lv<N>> subtractor_a{"subtractor_a"};
		sc_signal<sc_lv<N>> subtractor_b{"subtractor_b"};
		sc_signal<sc_logic> subtractor_bin{"subtractor_bin"};
		sc_signal<sc_lv<N>> subtractor_out{"subtractor_out"};
		sc_signal<sc_logic> subtractor_bout{"subtractor_bout"};
	};
}

#include "comparator.ipp"
