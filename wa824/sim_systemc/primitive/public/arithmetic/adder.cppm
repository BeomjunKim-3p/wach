module;
#include "sysc/tracing/sc_trace.h"
#include <systemc.h>

export module primitive.arithmetic.adder;


export namespace thpg::primitive::arithmetic {
	template <int N = 8>
	class Adder : public sc_module {
	public:
		sc_in<sc_lv<N>> a{"A"};
		sc_in<sc_lv<N>> b{"B"};
		sc_in<sc_logic> carry_in{"Carry In"};
		sc_out<sc_lv<N>> out{"out"};
		sc_out<sc_logic> carry_out{"Carry Out"};

		Adder(sc_module_name name, sc_trace_file *tf = nullptr);
		
		void refresh(void);
	};
}

#include "adder.ipp"
