template <int N>
thpg::primitive::arithmetic::bitwise::AndGate<N>::AndGate(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b;

	if (tf) {
		sc_trace(tf, a, (std::string(this->name()) + "(andN" + std::to_string(N) + ").a").c_str());
		sc_trace(tf, b, (std::string(this->name()) + "(andN" + std::to_string(N) + ").b").c_str());
		sc_trace(tf, out, (std::string(this->name()) + "(andN" + std::to_string(N) + ").out").c_str());
	}
}

template <int N>
void thpg::primitive::arithmetic::bitwise::AndGate<N>::refresh(void)
{
	[&]{
		if (!(a.read().is_01() && b.read().is_01())) {
			out.write(sc_lv<N>(SC_LOGIC_X));
			return;
		}
		
		out.write(a.read().to_uint() & b.read().to_uint());
		return;
	}();
}
