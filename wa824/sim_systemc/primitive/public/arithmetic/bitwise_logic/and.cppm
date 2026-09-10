module;
#include "sysc/tracing/sc_trace.h"
#include <systemc.h>

export module primitive.arithmetic.bitwise.and_gate;


export namespace thpg::primitive::arithmetic::bitwise {
	template <int N = 8>
	class AndGate : public sc_module {
	public:
		sc_in<sc_lv<N>> a{"A"};
		sc_in<sc_lv<N>> b{"B"};
		sc_out<sc_lv<N>> out{"out"};

		AndGate(sc_module_name name, sc_trace_file *tf = nullptr);
	
		void refresh(void);
	};
}

#include "and.ipp"
