template <int N, int W>
thpg::primitive::MuxNto1<N, W>::MuxNto1(sc_module_name name) 
    : sc_module(name), in("in", N) 
{
    SC_METHOD(refresh);
    sensitive << in << sel;
}

template <int N, int W>
void thpg::primitive::MuxNto1<N, W>::refresh() {
    if (sel.read().is_01() && sel.read().to_uint() < N) {
        out.write(in[sel.read().to_uint()].read());
    } else {
        out.write(sc_lv<W>(SC_LOGIC_X));
    }
} 
