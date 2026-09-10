template <int N, int W>
thpg::primitive::RegisterFileREdge<N, W>::RegisterFileREdge(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refreshcs);
	sensitive << addr << cs.pos();

	sc_bind(rregister.cs, rcs);
	sc_bind(rregister.alwaysout, ralwaysout);

	for (int i = 0; i < W; i++) {
		rregister[i].wr_bar(wr_bar);
		rregister[i].clk(clk);
		rregister[i].data(data);
	}

	if (tf) {
		sc_trace(tf, cs, (std::string(this->name()) + "(registerfileredge)" + ".cs").c_str());
		sc_trace(tf, addr, (std::string(this->name()) + "(registerfileredge)" + ".addr").c_str());
		sc_trace(tf, wr_bar, (std::string(this->name()) + "(registerfileredge)" + ".wr_bar").c_str());
		sc_trace(tf, clk, (std::string(this->name()) + "(registerfileredge)" + ".clk").c_str());
		sc_trace(tf, data, (std::string(this->name()) + "(registerfileredge)" + ".data").c_str());
		for (size_t i = 0; i < rcs.size(); ++i) {
			std::string trace_name = std::string(this->name()) + "(registerfileredge).rcs[" + std::to_string(i) + "]";
    			sc_trace(tf, rcs[i], trace_name.c_str());
		}
		sc_trace(tf, ralwaysout, (std::string(this->name()) + "(registerfileredge)" + ".ralwaysout").c_str());
	}
}

template <int N, int W>
void thpg::primitive::RegisterFileREdge<N, W>::refreshcs(void)
{
	for (int i = 0; i < W; i++) {
		rcs[i].write(SC_LOGIC_0);
	}
	
	if (!(cs.read() != SC_LOGIC_1) && addr.read().is_01()) {
		rcs[addr.read().to_uint()].write(SC_LOGIC_1);
	}
}
