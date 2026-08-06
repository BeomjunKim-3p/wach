module;
#include <systemc.h>

export module primitive.three_state_buffer;

export namespace thpg::primitive {
	class ThreeStateBuffer : public sc_module {
	public:
		sc_in<sc_logic> in{"in"};
		sc_in<bool> enable{"enable"};
		sc_out_resolved out{"out"};

		ThreeStateBuffer(sc_module_name name);

		void refresh(void); 
	};
}


