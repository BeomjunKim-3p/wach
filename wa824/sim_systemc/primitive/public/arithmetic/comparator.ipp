template <int N>
thpg::primitive::arithmetic::Comparator<N>::Comparator(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << a << b;

	
	subtractor.a(subtractor_a);
	subtractor.b(subtractor_b);
	subtractor.borrow_in(subtractor_bin);
	subtractor.out(subtractor_out);
	subtractor.borrow_out(subtractor_bout);
	a(subtractor_a);
	b(subtractor_b);
	subtractor_bin.write(SC_LOGIC_0);
	
	if (tf) {
		sc_trace(tf, a, (std::string(this->name()) + "(comparatorN" + std::to_string(N) + ").a").c_str());
		sc_trace(tf, b, (std::string(this->name()) + "(comparatorN" + std::to_string(N) + ").b").c_str());
		sc_trace(tf, less, (std::string(this->name()) + "(comparatorN" + std::to_string(N) + ").less").c_str());
		sc_trace(tf, equal, (std::string(this->name()) + "(comparatorN" + std::to_string(N) + ").equal").c_str());
		sc_trace(tf, greater, (std::string(this->name()) + "(comparatorN" + std::to_string(N) + ").greater").c_str());
	}
}

template <int N>
void thpg::primitive::arithmetic::Comparator<N>::refresh(void)
{
	[&]{
		// 예외처리
		if (!subtractor_out.read().is_01()) {
			less.write(SC_LOGIC_X);
			equal.write(SC_LOGIC_X);
			greater.write(SC_LOGIC_X);
			return;
		}
		
		// zero
		if (sc_int<N>(subtractor_out.read()).to_int() == 0) {
			less.write(SC_LOGIC_0);
			equal.write(SC_LOGIC_1);
			greater.write(SC_LOGIC_0);
			return;
		}
		
		if (subtractor_bout.read().to_bool()) {
			// less
			less.write(SC_LOGIC_1);
			equal.write(SC_LOGIC_0);
			greater.write(SC_LOGIC_0);
		} else {
			// greater
			less.write(SC_LOGIC_0);
			equal.write(SC_LOGIC_0);
			greater.write(SC_LOGIC_1);
		}
	}();
}
