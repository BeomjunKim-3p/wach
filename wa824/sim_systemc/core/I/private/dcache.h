#pragma once
#include <systemc.h>

// D-Cache: 256 lines, 1byte/line, direct-mapped, write-back write-allocate
// 메모리 인터페이스: MemArbiter 경유

namespace thpg::wa824 {
	class DCache : public sc_module {
	public:
		sc_in<sc_lv<16>>  addr      {"ADDR"};
		sc_in<sc_lv<8>>   wdata     {"WDATA"};
		sc_in<bool>       mem_read  {"MEM_READ"};
		sc_in<bool>       mem_write {"MEM_WRITE"};
		sc_out<sc_lv<8>>  rdata     {"RDATA"};
		sc_out<bool>      hit       {"HIT"};

		sc_in<bool>       clk       {"CLK"};
		sc_in<bool>       reset     {"RESET"};

		// MemArbiter 인터페이스
		sc_out<sc_lv<16>> dc_addr   {"DC_ADDR"};
		sc_out<sc_lv<8>>  dc_wdata  {"DC_WDATA"};
		sc_out<bool>      dc_we     {"DC_WE"};
		sc_out<bool>      dc_re     {"DC_RE"};
		sc_in<sc_lv<8>>   dc_rdata  {"DC_RDATA"};
		sc_in<bool>       dc_ready  {"DC_READY"};
		sc_in<bool>       dc_grant  {"DC_GRANT"};

		DCache(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh_read(void);  // combinational
		void refresh_wb(void);    // sequential (clk.pos())

	private:
		sc_uint<9> tag_store[256];
		bool       dirty[256];
		sc_uint<8> data_store[256];

		// write-back 상태
		bool wb_pending{"wb_pending"};
		sc_uint<16> wb_addr_buf{"wb_addr_buf"};
		sc_uint<8>  wb_data_buf{"wb_data_buf"};
	};
}
