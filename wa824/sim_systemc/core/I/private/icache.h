#pragma once
#include <systemc.h>

// I-Cache: 256 lines, 3byte/line, direct-mapped
// Tag: 9bit (valid 1 + tag 8), Even/Odd bank 인터리빙
// Data: 3개 bank 각 256x8bit (behavioral)
// Boundary crossing: pc%3 != 0 이면 index/index+1 양쪽 참조
//
// 메모리 인터페이스: MemArbiter 경유
//   miss 발생 → ic_re=1, ic_addr=요청주소
//   mem_arbiter가 grant=1 + ready=1 → ic_rdata로 1바이트씩 수신
//   3바이트 수신 완료 후 캐시 채우기

namespace thpg::wa824 {
	class ICache : public sc_module {
	public:
		sc_in<sc_lv<16>>  pc       {"PC"};
		sc_out<sc_lv<24>> inst     {"INST"};
		sc_out<bool>      hit      {"HIT"};

		sc_in<bool>       clk      {"CLK"};
		sc_in<bool>       reset    {"RESET"};

		// MemArbiter 인터페이스
		sc_out<sc_lv<16>> ic_addr  {"IC_ADDR"};
		sc_out<bool>      ic_re    {"IC_RE"};
		sc_in<sc_lv<8>>   ic_rdata {"IC_RDATA"};
		sc_in<bool>       ic_ready {"IC_READY"};
		sc_in<bool>       ic_grant {"IC_GRANT"};

		ICache(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh_hit(void);   // combinational
		void refresh_fill(void);  // sequential (clk.pos())

	private:
		sc_uint<9> tag_even[128];
		sc_uint<9> tag_odd[128];
		sc_uint<8> data_b0[256];
		sc_uint<8> data_b1[256];
		sc_uint<8> data_b2[256];

		// miss 처리 상태: 3바이트 순차 수신
		sc_uint<2> fill_state{"fill_state"}; // 0=idle,1=byte0,2=byte1,3=byte2
		sc_uint<8> fill_buf[3];
		sc_uint<16> fill_addr{"fill_addr"};
	};
}
