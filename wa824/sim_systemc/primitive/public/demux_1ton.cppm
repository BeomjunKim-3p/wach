module;
#include <systemc.h>
#include <cmath>

export module primitive.demux_1ton;

#define CLOG2(N) ((N) <= 1 ? 1 : static_cast<int>(std::bit_width(static_cast<unsigned int>((N) - 1))))

export namespace thpg::primitive {
	template<int N = 2, int W = 8>
	class DeMux1toN : public sc_module {
	public:
		sc_in<sc_lv<W>> in{"in"};
		static constexpr int SEL_WIDTH = CLOG2(N);
		sc_in<sc_lv<SEL_WIDTH>> sel{"sel"};
		sc_vector<sc_out<sc_lv<W>>> out{"out", N};

		DeMux1toN(sc_module_name name);

		void refresh(void);
	};
}

#include "demux_1ton.ipp"
