template <int N>
thpg::primitive::arithmetic::Shifter<N>::Shifter(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b << is_rotate << direction_right;
	
	if (tf) {
		sc_trace(tf, a, (std::string(this->name()) + "(shifterN" + std::to_string(N) + ").a").c_str());
		sc_trace(tf, b, (std::string(this->name()) + "(shifterN" + std::to_string(N) + ").b").c_str());
		sc_trace(tf, is_rotate, (std::string(this->name()) + "(shifterN" + std::to_string(N) + ").is_rotate").c_str());
		sc_trace(tf, direction_right, (std::string(this->name()) + "(shifterN" + std::to_string(N) + ").direction_right").c_str());
		sc_trace(tf, out, (std::string(this->name()) + "(shifterN" + std::to_string(N) + ").out").c_str());
	}
}

template <int N>
void thpg::primitive::arithmetic::Shifter<N>::refresh(void)
{
	[&]{
		// 예외처리
		if (!(a.read().is_01() && b.read().is_01())) {
			out.write(sc_lv<N>(SC_LOGIC_X));
			return;
		}

		if (is_rotate && direction_right) {
			out.write((a >> b) | (a << (N - b)));
			return;
		}
		if (is_rotate && !direction_right) {
			out.write((a << b) | (a >> (N - b)));
			return;
		}
		if (!is_rotate && direction_right) {
			out.write(a >> b);
			return;
		}
		if (!is_rotate && !direction_right) {
			out.write(a << b);
			return;
		}
	}();
}
