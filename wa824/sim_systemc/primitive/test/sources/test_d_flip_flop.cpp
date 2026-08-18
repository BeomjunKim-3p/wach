#include "sysc/kernel/sc_simcontext.h"
#include "sysc/tracing/sc_trace.h"
#include <random>
#include <systemc.h>
#include <iostream>

import primitive.d_flip_flop;


#define TEST_ITERATIONS 10000

int sc_main(int argc, char* argv[]) {
	
	//
	// 사전준비
	//
	
	sc_signal<sc_logic> d;
	sc_signal<bool> clk;
	sc_signal<sc_logic> q;
	sc_signal<sc_logic> q_bar;

	thpg::primitive::DFlipFlop dut("dut");
	dut.d(d);
	dut.clk(clk);
	dut.q(q);
	dut.q_bar(q_bar);

	sc_trace_file *tf = sc_create_vcd_trace_file("vcd_d_flip_flop_test");
	tf->set_time_unit(10, SC_NS);
	sc_trace(tf, d, "d");
	sc_trace(tf, clk, "clk");
	sc_trace(tf, q, "q");
	sc_trace(tf, q_bar, "q_bar");

	//
	// 테스트
	//
	
	bool test_passed = true;

	{ //대충이상한짓거르기
	{ //D FlipFlop에 Z넣는사람이 어딨어
		clk.write(false);
		d.write(SC_LOGIC_Z);
		sc_start(10, SC_NS);
		clk.write(true);
		sc_start(10, SC_NS);

		if (q.read() != SC_LOGIC_X) {
			std::cerr << "[FAIL] Exception Case Test Z: Expected Q == SC_LOGIC_X, but Q == SC_LOGIC_" << q.read().to_char() << std::endl;
		}
		if (q_bar.read() != SC_LOGIC_X) {
			std::cerr << "[FAIL] Exception Case Test Z: Expected ̅Q == SC_LOGIC_X, but ̅Q == SC_LOGIC_" << q_bar.read().to_char() << std::endl;
		}
	}
	//대충초기화
	d.write(SC_LOGIC_0);
	clk.write(true);
	sc_start(10, SC_NS);
	clk.write(false);

	{ //D FlipFlop에 X넣는사람이 어딨어
		clk.write(false);
		d.write(SC_LOGIC_X);
		sc_start(10, SC_NS);
		clk.write(true);
		sc_start(10, SC_NS);

		if (q.read() != SC_LOGIC_X) {
			std::cerr << "[FAIL] Exception Case Test X: Expected Q == SC_LOGIC_X, but Q == SC_LOGIC_" << q.read().to_char() << std::endl;
		}
		if (q_bar.read() != SC_LOGIC_X) {
			std::cerr << "[FAIL] Exception Case Test X: Expected ̅Q == SC_LOGIC_X, but ̅Q == SC_LOGIC_" << q_bar.read().to_char() << std::endl;
		}
	}
	}
	
	//대충초기화
	d.write(SC_LOGIC_0);
	clk.write(true);
	sc_start(10, SC_NS);
	clk.write(false);

	{ // 대충난수테스트
	std::random_device rd;
	std::mt19937 rng(rd());
	std::bernoulli_distribution bitDist(0.5);

	bool prev_clk = false, expected_q = false, prev_q = false;
	
	

	clk.write(prev_clk);
	d.write(SC_LOGIC_0);
	sc_start(10, SC_NS);

	for (int i = 0; i <= TEST_ITERATIONS; i++) {
		bool test_clk = bitDist(rng);
		bool test_d = bitDist(rng);
		prev_q = expected_q;
		expected_q = ((prev_clk == 0) && (test_clk == 1)) ? test_d : prev_q;
		d.write(test_d ? SC_LOGIC_1 : SC_LOGIC_0);
		clk.write(test_clk);
		if (!q.read().is_01()) {
			std::cerr << "[FAIL] In Random Test Iteration " << i << " !Q.read().is_01()" << std::endl;
			break;
		}
		if (!q_bar.read().is_01()) {
			std::cerr << "[FAIL] In Random Test Iteration " << i << " !̅Q.read().is_01()" << std::endl;
			break;
		}
		if (q.read().to_bool() != expected_q) {
			std::cerr << "[FAIL] In Random Test Iteration " << i << " Expected Q = " << (expected_q ? "SC_LOGIC_1" : "SC_LOGIC_0") << " Value = SC_LOGIC_" << q.read().to_char() << std:: endl;
		}
		if (q_bar.read().to_bool() == expected_q) {
			std::cerr << "[FAIL] In Random Test Iteration " << i << " Expected ̅Q = " << (expected_q ? "SC_LOGIC_0" : "SC_LOGIC_1") << " Value = SC_LOGIC_" << q_bar.read().to_char() << std:: endl;
		}
	}
	
	}

	sc_close_vcd_trace_file(tf);
	
	if (test_passed) {
		std::cout << ">>> ALL TESTS PASSED <<<" << std::endl;
		return 0;
	} else {
		std::cerr << ">>> TEST FAILED <<<" << std::endl;
return 1;
	}
}

