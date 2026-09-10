template <int N>
thpg::primitive::arithmetic::Adder<N>::Adder(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b;
	
	if (tf) {
		sc_trace(tf, a, (std::string(this->name()) + "(adderN" + std::to_string(N) + ").a").c_str());
		sc_trace(tf, b, (std::string(this->name()) + "(adderN" + std::to_string(N) + ").b").c_str());
		sc_trace(tf, carry_in, (std::string(this->name()) + "(adderN" + std::to_string(N) + ").carry_in").c_str());
		sc_trace(tf, out, (std::string(this->name()) + "(adderN" + std::to_string(N) + ").out").c_str());
		sc_trace(tf, carry_out, (std::string(this->name()) + "(adderN" + std::to_string(N) + ").carry_out").c_str());
	}
}

template <int N>
void thpg::primitive::arithmetic::Adder<N>::refresh(void)
{
	using u_int_type = typename std::conditional_t<(N < 64), sc_uint<N + 1>, sc_biguint<N + 1>>;

	[&]{
		if (!(a.read().is_01() && b.read().is_01() && carry_in.read().is_01())) {
			out.write(sc_lv<N>(SC_LOGIC_X));
			carry_out.write(SC_LOGIC_X);
			return;
		}

		u_int_type val_a = a.read(), val_b = b.read();
		u_int_type add_res = val_a + val_b + carry_in.read().to_bool();

		out.write(add_res.range(N - 1, 0));
		carry_out.write(add_res[N]);
		return;
	}();
}
