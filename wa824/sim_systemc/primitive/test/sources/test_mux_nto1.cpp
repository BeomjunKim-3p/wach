#include <cstdint>
#include <string>
#include <systemc.h>
#include <iostream>
#include <random>
#include <bit>

import primitive.mux_nto1;

#define CLOG2(N) ((N) <= 1 ? 1 : static_cast<int>(std::bit_width(static_cast<unsigned int>((N) - 1))))

template <int N, int W>
struct MuxTest {
	//
	// 사전준비
	//
	
	static constexpr int SEL_WIDTH = CLOG2(N);
	sc_vector<sc_signal<sc_lv<W>>> in;
	sc_signal<sc_lv<SEL_WIDTH>> sel;
	sc_signal<sc_lv<W>> out;

	thpg::primitive::MuxNto1<N, W> dut;

	// VCD생성
	std::string filename = "vcd_mux_test_N" + std::to_string(N) + "_W" + std::to_string(W);
	sc_trace_file *tf = sc_create_vcd_trace_file(filename.c_str());
	
	MuxTest(const char* name)
		: in((std::string(name) + "_in").c_str(), N)
		, sel((std::string(name) + "_sel").c_str())
		, out((std::string(name) + "_out").c_str())
		, dut(name)
	{
		tf->set_time_unit(1, SC_NS);
		for (int i = 0; i < N; i++) {
			std::string name = "in(" + std::to_string(i) + ")";
			sc_trace(tf, in[i], name);
		}
		sc_trace(tf, sel, "sel");
		sc_trace(tf, out, "out");

		// DUT 인스턴스 생성 & 배선
		dut.in(in);
		dut.sel(sel);
		dut.out(out);
	}
	bool run_test(int test_iterations, std::mt19937 &rng)
	{
		uint64_t max_data_val = (W >= 64) ? UINT64_MAX : ((1ULL << W) - 1);
		std::uniform_int_distribution<uint64_t> data_dist(0, max_data_val);
		std::uniform_int_distribution<int> sel_dist(0, N - 1);
	
		// 
		// 테스트
		//
		
		sel.write(sc_lv<SEL_WIDTH>(SC_LOGIC_X));
		sc_start(10, SC_NS);
	
		if (out.read().is_01()) {
			std::cerr << "[FAIL] Initial 'X' selection check failed" << std::endl;
			sc_close_vcd_trace_file(tf);
			return false;
		}
	
		// 난수 테스트
		std::vector<uint64_t> random_in(N);
	
		for (int iter = 0; iter < test_iterations; ++iter) {
			for (int i = 0; i < N; ++i) {
				random_in[i] = data_dist(rng);
				in[i].write(random_in[i]);
			}

			int selidx = sel_dist(rng);
			sel.write(selidx);

			sc_start(10, SC_NS);
	
			uint64_t expected_val = random_in[selidx];
			
			if (!out.read().is_01() || out.read().to_uint64() != expected_val) {
				std::cerr << "[ERROR Iteration " << iter << "]\n"
						  << "	Selected sel_idx : " << selidx << "\n"
						  << "	Expected Value   : 0x" << std::hex << expected_val << "\n"
						  << "	Out Value : 0x" << out.read().to_uint64() << std::dec << "\n";
				sc_close_vcd_trace_file(tf);
				return false;
			}
		}

		// VCD 출력
		sc_close_vcd_trace_file(tf);
		return true;
	
	}
};


int sc_main(int argc, char* argv[])
{

	std::random_device rd;
	std::mt19937 rng(rd());
	
	MuxTest<2, 8> test_8bit_2to1("test_8bit_2to1");
	MuxTest<4, 16> test_4bit_16to1("test_4bit_16to1");
	MuxTest<3, 8> test_3bit_8to1("test_3bit_8to1");
	MuxTest<8, 32> test_8bit_32to1("test_8bit_32to1");
	bool test_passed = true;

	test_passed &= test_8bit_2to1.run_test(50, rng);
	test_passed &= test_4bit_16to1.run_test(50, rng);
	test_passed &= test_3bit_8to1.run_test(50, rng);
	test_passed &= test_8bit_32to1.run_test(50, rng);

	if (test_passed) {
		std::cout << ">>> ALL TESTS PASSED <<<" << std::endl;
		return 0;
	} else {
		std::cerr << ">>> TEST FAILED <<<" << std::endl;
		return 1;
	}
}

