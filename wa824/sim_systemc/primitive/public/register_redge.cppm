module;
#include <systemc.h>

export module primitive.register_redge;


export namespace thpg::primitive {
	template <int N>
	using u_int_type = typename std::conditional_t<(N < 64), sc_uint<N>, sc_biguint<N>>;

	template <int N = 8>
	class RegisterREdge : public sc_module {
	public:
		sc_in<sc_logic> cs{"CS"};
		sc_in<sc_logic> wr_bar{"WR\'"};
		sc_in<bool> clk{"CLK"};
		sc_inout_rv<N> data{"DATA"};
		sc_out<sc_lv<N>> alwaysout{"ALWAYSOUT"};

		RegisterREdge(sc_module_name name);

		// 레지스터가 아닌 사용하는쪽 입장에서의 함수명
		//// 레지스터가 출력
		void read(void);
		//// 레지스터가 읽음
		void write(void);
	
	private:
		u_int_type<N> register_data;
	};
}

#include "register_redge.ipp"
