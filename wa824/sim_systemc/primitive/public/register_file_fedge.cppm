module;
#include <systemc.h>

export module primitive.register_file_fedge;

import primitive.register_fedge;

#define CLOG2(N) ((N) <= 1 ? 1 : static_cast<int>(std::bit_width(static_cast<unsigned int>((N) - 1))))

export namespace thpg::primitive {
	template<int N = 8, int W = 16>
	class RegisterFileFEdge : public sc_module {
	public:
		sc_in<sc_logic> cs{"CS"};
		static constexpr int ADDR_WIDTH = CLOG2(W);
		sc_in<sc_lv<ADDR_WIDTH>> addr{"ADDR"};
		sc_in<sc_logic> wr_bar{"WR\'"};
		sc_in<bool> clk{"CLK"};
		sc_inout_rv<N> data{"DATA"};

		RegisterFileFEdge(sc_module_name name, sc_trace_file *tf = nullptr);

		void refreshcs(void);
	
	private:
	// 대충 rcs만 demux통과시키고 나머지 다 묶기
		sc_vector<RegisterFEdge<N>> rregister{"R REGISTER"};
		sc_vector<sc_signal<sc_logic>> rcs{"R CS", W};
		sc_vector<sc_signal<sc_lv<N>>> ralwaysout{"R ALWAYSOUT", W};
	};
}

#include "register_file_fedge.ipp"
