#include "sysc/communication/sc_signal_resolved.h"
#include <systemc.h>
#include <iostream>

import primitive.three_state_buffer;

int sc_main(int argc, char* argv[]) {
	
	//
	// 사전준비
	//
	
	sc_signal<sc_logic> in;
	sc_signal<bool> enable;
	sc_signal_resolved out;

	// 2. DUT 인스턴스 생성 & 배선
	thpg::primitive::ThreeStateBuffer dut("dut");
	dut.in(in);
	dut.enable(enable);
	dut.out(out);

	// 시뮬레이션 트레이스 파일(VCD) 생성
	sc_trace_file* tf = sc_create_vcd_trace_file("vcd_three_state_buffer_test");
	sc_trace(tf, in, "in");
	sc_trace(tf, enable, "enable");
	sc_trace(tf, out, "out");

	bool test_passed = true;

	//
	// 테스트
	//

	// [Test Case 1-1] if (in == 0) && (enable == false) then (out = Z)
	in.write(SC_LOGIC_0);
	enable.write(false);
	sc_start(10, SC_NS); // simulate

	if (out.read() != SC_LOGIC_Z) {
		test_passed = false;
		std::cerr << "[ERROR] Test Case 1-1 Failed: Expected (out == 'SC_LOGIC_Z'), but got (out == 'SC_LOGIC_"
			<< out.read() << "'). (in = 'SC_LOGIC_0', enable = 'false')" << std::endl;
	}

	// [Test Case 1-2] if (in == 1) && (enable == false) then (out = Z)
	in.write(SC_LOGIC_1);
	enable.write(false);
	sc_start(10, SC_NS); // simulate

	if (out.read() != SC_LOGIC_Z) {
		test_passed = false;
		std::cerr << "[ERROR] Test Case 1-2 Failed: Expected (out == 'SC_LOGIC_Z'), but got (out == 'SC_LOGIC_"
			<< out.read() << "'). (in = 'SC_LOGIC_1', enable = 'false')" << std::endl;
	}
	
	// [Test Case 2] if (in == 0) && (enable == true) then (out = 0)
	in.write(SC_LOGIC_0);
	enable.write(true);
	sc_start(10, SC_NS); // simulate

	if (out.read() != SC_LOGIC_0) {
		test_passed = false;
		std::cerr << "[ERROR] Test Case 1-1 Failed: Expected (out == 'SC_LOGIC_0'), but got (out == 'SC_LOGIC_"
			<< out.read() << "'). (in = 'SC_LOGIC_0', enable = 'true')" << std::endl;
	}

	// [Test Case 2] if (in == 1) && (enable == true) then (out = 1)
	in.write(SC_LOGIC_1);
	enable.write(true);
	sc_start(10, SC_NS); // simulate

	if (out.read() != SC_LOGIC_1) {
		test_passed = false;
		std::cerr << "[ERROR] Test Case 1-2 Failed: Expected (out == 'SC_LOGIC_1'), but got (out == 'SC_LOGIC_"
			<< out.read() << "'). (in = 'SC_LOGIC_1', enable = 'true')" << std::endl;
	}

	// VCD 출력
	sc_close_vcd_trace_file(tf);

	//
	// CTest 통과 여부 반환
	//
	
	if (test_passed) {
		std::cout << ">>> ALL TESTS PASSED <<<" << std::endl;
		return 0;
	} else {
		std::cerr << ">>> TEST FAILED <<<" << std::endl;
		return 1;
	}
}

