template<int N, int W>
thpg::primitive::DeMux1toN<N,W>::DeMux1toN(sc_module_name name) 
	:sc_module(name) 
{
	SC_METHOD(refresh);
	sensitive<< in << sel;
}

template<int N, int W>
void thpg::primitive::DeMux1toN<N, W>::refresh() {
	/*if(sel.read().is_01() && sel.read().to_uint() < N) {
		out.write(in[sel.read().to_uint()].read());
	}else {
		out.write(sc_lv<W>(SC_LOGIC_X));
	}*/
	if(sel.read().is_01() && sel.read().to_uint() < N) {
		for (int i = 0; i < N; i++) {
			out[i].write(sc_lv<W>(SC_LOGIC_Z));
		}
		out[sel.read().to_uint()].write(in.read());
	}else {
		for (int i = 0; i < N; i++) {
			out[i].write(sc_lv<W>(SC_LOGIC_X));
		}
	}
}
