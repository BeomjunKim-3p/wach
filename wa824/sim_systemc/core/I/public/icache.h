#pragma once
#include <systemc.h>

// I-Cache: 256 lines, 3byte/line, direct-mapped
// Line index: (pc/3) & 0xFF, tag: (pc/3) >> 8
// Tag: 9bit (valid 1 + tag 8), Even/Odd bank 인터리빙
// Data: 3개 bank 각 256x8bit (behavioral)
// Boundary crossing: pc%3 != 0 이면 현재 line과 다음 line을 조합
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

		// 디버거가 특정 주소가 포함된 라인(또는 전체)을 무효화할 때 사용한다.
		// 수정된 바이트를 포함할 수 있는 명령어는 최대 두 바이트 앞에서
		// 시작할 수 있으므로 addr-2..addr가 속한 라인을 함께 무효화한다.
		void invalidate_line(uint16_t addr);
		void invalidate_all(void);

	private:
		sc_uint<9> tag_even[128];
		sc_uint<9> tag_odd[128];
		sc_uint<8> data_b0[256];
		sc_uint<8> data_b1[256];
		sc_uint<8> data_b2[256];

		// miss 처리 상태: 3바이트 순차 수신
		// 0=다음 바이트가 byte0, 1=byte1 대기, 2=byte2 대기
		sc_uint<2> fill_state{0};
		sc_uint<8> fill_buf[3];
		sc_uint<16> fill_addr{0}; // 현재 채우는 라인의 3바이트 정렬 시작 주소
		sc_event   fill_done_;
		sc_event   invalidate_event_;
		sc_event   fill_restart_done_;
		bool       fill_restart{false};
	};
}
