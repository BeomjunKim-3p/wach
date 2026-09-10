template <int N>
thpg::primitive::RegisterREdge<N>::RegisterREdge(sc_module_name name)
	: sc_module(name)
{
	SC_METHOD(read);
	sensitive << cs << wr_bar;

	SC_METHOD(write);
	sensitive << clk.pos();
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
		if (cs.read() != SC_LOGIC_1) return;
		if (wr_bar.read() != SC_LOGIC_1) return;

		// 칩선택 && write상태 && rising edge
		register_data = data.read();

		// 어짜피별이상한일이있지않는한 ALWAYSOUT은 write때만 값 바뀌므로 AlwaysOut 업데이트
		alwaysout.write(sc_lv<N>(register_data));
	}();
}
