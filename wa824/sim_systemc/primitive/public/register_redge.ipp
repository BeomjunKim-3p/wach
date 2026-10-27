template <int N>
thpg::primitive::RegisterREdge<N>::RegisterREdge(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(read);
	sensitive << cs << wr_bar << clk.pos();

	SC_METHOD(write);
	sensitive << clk.pos();
	
	std::string module_name = std::string(this->name()) + "(RegisterREdgeN" + std::to_string(N) + ")";

	if (tf) {
		sc_trace(tf, cs, (module_name + ".CS").c_str());
		sc_trace(tf, wr_bar, (module_name + ".WR\'").c_str());
		sc_trace(tf, clk, (module_name + ".CLK").c_str());
		sc_trace(tf, data, (module_name + ".DATA").c_str());
		sc_trace(tf, reset, (module_name + ".RESET").c_str());
		sc_trace(tf, alwaysout, (module_name + ".AlwaysOut").c_str());
	}
}

template <int N>
void thpg::primitive::RegisterREdge<N>::read(void)
{
	[&] {
		if (cs.read() != SC_LOGIC_1) {
			// 선택되지 않았으니 HI_Z출력
			data.write(sc_lv<N>(SC_LOGIC_Z));
			return;
		}
		if (wr_bar.read() != SC_LOGIC_0) {
			// 선택은 되었는데 write상태나 unknown상태이므로 출력 안함
			data.write(sc_lv<N>(SC_LOGIC_Z));
			return;
		}

		// 칩 선택 && read상태
		data.write(sc_lv<N>(register_data));
	}();
}

template <int N>
void thpg::primitive::RegisterREdge<N>::write(void)
{
	[&]{
		// reset은 cs/wr_bar 상태와 무관하게 레지스터 내용을 0으로 만든다.
		// (cs가 0인 동안 alwaysout이 초기값 X로 남는 문제 방지)
		if (reset.read() == true) {
			register_data = 0;
			alwaysout.write(sc_lv<N>(0));
			return;
		}
		if (cs.read() != SC_LOGIC_1) return;
		if (wr_bar.read() != SC_LOGIC_1) return;

		// 칩선택 && write상태 && rising edge
		register_data = data.read();

		// 어짜피별이상한일이있지않는한 ALWAYSOUT은 write때만 값 바뀌므로 AlwaysOut 업데이트
		alwaysout.write(sc_lv<N>(register_data));
	}();
}
