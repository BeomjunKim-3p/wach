module;
#include "sysc/tracing/sc_trace.h"
#include <systemc.h>

export module primitive.arithmetic.shifter;


export namespace thpg::primitive::arithmetic {
	template <int N = 8>
	class Shifter : public sc_module {
	public:
		sc_in<sc_lv<N>> a{"A"};
		sc_in<sc_lv<N>> b{"B"};
		sc_in<bool> is_rotate{"rotate"};
		sc_in<bool> direction_right{"direction right"};
		sc_out<sc_lv<N>> out{"out"};

		Shifter(sc_module_name name, sc_trace_file *tf = nullptr);
		
		void refresh(void);
	};
}

#include "shifter.ipp"
