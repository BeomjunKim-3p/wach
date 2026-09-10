module;
#include <iomanip>
#include <systemc.h>
#include <type_traits>

export module primitive.arithmetic.subtractor;


export namespace thpg::primitive::arithmetic {
	template <int N = 8>
	class Subtractor : public sc_module {
	public:
		sc_in<sc_lv<N>> a{"A"};
		sc_in<sc_lv<N>> b{"B"};
		sc_in<sc_logic> borrow_in{"Borrow In"};
		sc_out<sc_lv<N>> out{"out"};
		sc_out<sc_logic> borrow_out{"Borrow Out"};

		Subtractor(sc_module_name name, sc_trace_file *tf = nullptr);
	
		void refresh(void);
	};
}

#include "subtractor.ipp"
