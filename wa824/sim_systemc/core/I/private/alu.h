#include <systemc.h>

import primitive.arithmetic.bitwise.and_gate;
import primitive.arithmetic.bitwise.or_gate;
import primitive.arithmetic.bitwise.xor_gate;
import primitive.arithmetic.adder;
import primitive.arithmetic.subtractor;
import primitive.arithmetic.shifter;
import primitive.arithmetic.comparator;
import primitive.mux_nto1;

namespace thpg::core::i {
	class ALU : public sc_module {
	public:
		sc_in<sc_lv<8>> a{"A"};
		sc_in<sc_lv<8>> b{"B"};
		// 설계도상으로는 alu플래그 4비트이지만 carry외에 사용되지 않고 내부 연산장치들에게 넘겨줄 플래그 필요해 변경
		sc_in<sc_logic> carry{"Carry"};
		sc_in<sc_logic> invert{"invert"};
		sc_in<bool> is_rotate{"is_rotate"};
		sc_in<bool> direction_right{"direction_right"};
		sc_in<sc_lv<4>> inst_in{"Inst In"};
		sc_out<sc_lv<8>> y{"Y"};
		sc_out<sc_lv<4>> flag_out{"Flag Out"};

		ALU(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
		void cmpref(void);
		void invref(void);
	
	private:
		thpg::primitive::arithmetic::bitwise::AndGate<8> andgate;
		sc_signal<sc_lv<8>> andout{"andout"};

		thpg::primitive::arithmetic::bitwise::OrGate<8> orgate;
		sc_signal<sc_lv<8>> orout{"orout"};

		thpg::primitive::arithmetic::bitwise::XorGate<8> xorgate;
		sc_signal<sc_lv<8>> xorout{"xorout"};

		thpg::primitive::arithmetic::Adder<8> adder;
		sc_signal<sc_logic> carryout{"CarryOut"};
		sc_signal<sc_lv<8>> adderout{"adderout"};
		sc_signal<sc_logic> addercarryout{"addercarryout"};
		sc_signal<sc_lv<8>> invb{"invb"};

		thpg::primitive::arithmetic::Subtractor<8> subtractor;
		sc_signal<sc_logic> borrowout{"BorrowOut"};
		sc_signal<sc_lv<8>> subtractorout{"subtractorout"};
		sc_signal<sc_logic> subtractorcarryout{"subtractorcarryout"};
		
		thpg::primitive::arithmetic::Shifter<8> shifter;
		sc_signal<sc_lv<8>> shifterout{"shifterout"};

		thpg::primitive::arithmetic::Comparator<8> comparator;
		sc_signal<sc_logic> less{"Less"};
		sc_signal<sc_logic> equal{"Equal"};
		sc_signal<sc_logic> greater{"Greater"};
		sc_signal<sc_lv<8>> cmpout{"cmpout"};

		thpg::primitive::MuxNto1<7, 8> outmux;
		sc_signal<sc_lv<8>> muxout{"muxout"};
		sc_signal<sc_lv<3>> muxsel{"muxsel"};
	};
}


