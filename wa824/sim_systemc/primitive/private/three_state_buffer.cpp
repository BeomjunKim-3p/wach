module;
#include <systemc.h>
#include <unistd.h>

module primitive.three_state_buffer;

namespace thpg::primitive {
	ThreeStateBuffer::ThreeStateBuffer(sc_module_name name) : sc_module(name) {
		SC_METHOD(refresh);
		sensitive << in << enable;
	}


	void ThreeStateBuffer::refresh(void)
	{
		if (enable.read() == true) {
			out.write(in.read());
		} else {
			out.write(SC_LOGIC_Z);
		}
	}
}


