template <int N>
thpg::primitive::arithmetic::Subtractor<N>::Subtractor(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b;
	
	if (tf) {
		sc_trace(tf, a, (std::string(this->name()) + "(subtractorN" + std::to_string(N) + ").a").c_str());
		sc_trace(tf, b, (std::string(this->name()) + "(subtractorN" + std::to_string(N) + ").b").c_str());
		sc_trace(tf, borrow_in, (std::string(this->name()) + "(subtractorN" + std::to_string(N) + ").borrow_in").c_str());
		sc_trace(tf, out, (std::string(this->name()) + "(subtractorN" + std::to_string(N) + ").out").c_str());
		sc_trace(tf, borrow_out, (std::string(this->name()) + "(subtractorN" + std::to_string(N) + ").borrow_out").c_str());
	}
}

template <int N>
void thpg::primitive::arithmetic::Subtractor<N>::refresh(void)
{
	using u_int_type = typename std::conditional_t<(N < 64), sc_uint<N + 1>, sc_biguint<N + 1>>;
	
	[&]{
		if (!(a.read().is_01() && b.read().is_01() && borrow_in.read().is_01())) {
			out.write(sc_lv<N>(SC_LOGIC_X));
			borrow_out.write(SC_LOGIC_X);
			return;
		}

		u_int_type val_a = a.read(), val_b = b.read();
		u_int_type sub_res = val_a - val_b - borrow_in.read().to_bool();

		out.write(sub_res.range(N - 1, 0));
		borrow_out.write(sub_res[N]);
	}();
}
