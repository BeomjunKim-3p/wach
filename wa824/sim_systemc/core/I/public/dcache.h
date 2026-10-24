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

		void refresh_read(void);   // combinational request generator
		void refresh_wb(void);     // sequential (clk.pos())
		void refresh_output(void); // single driver for dc_* outputs

		// 디버거가 메인 메모리를 수정한 뒤 해당 라인/전체를 무효화한다.
		// 진행 중인 write-back이 있으면 다음 클럭에서 취소한다.
		void invalidate_line(uint16_t addr);
		void invalidate_all(void);

	private:
		sc_uint<9> tag_store[256];
		bool       dirty[256];
		sc_uint<8> data_store[256];

		// refresh_read와 refresh_wb가 dc_* 포트를 직접 구동하면 sc_signal
		// 다중 드라이버가 되므로, 내부 요청을 모은 뒤 refresh_output에서
		// 한 번만 포트를 구동한다.
		sc_signal<bool>      req_we{"REQ_WE"};
		sc_signal<bool>      req_re{"REQ_RE"};
		sc_signal<sc_lv<16>> req_addr{"REQ_ADDR"};
		sc_signal<sc_lv<8>>  req_wdata{"REQ_WDATA"};

		sc_signal<bool>      wb_active{"WB_ACTIVE"};
		sc_signal<bool>      wb_we{"WB_WE"};
		sc_signal<bool>      wb_re{"WB_RE"};
		sc_signal<sc_lv<16>> wb_addr{"WB_ADDR"};
		sc_signal<sc_lv<8>>  wb_wdata{"WB_WDATA"};

		// write-back 상태
		bool wb_pending{false};
		sc_uint<16> wb_addr_buf{0};
		sc_uint<8>  wb_data_buf{0};
		sc_event    fill_done_;
		sc_event    invalidate_event_;

		// 디버그 무효화로 취소할 write-back. 실제 신호는 refresh_wb만
		// 구동하고, refresh_output은 이 플래그를 보고 요청을 억제한다.
		bool        wb_cancel_valid{false};
		bool        wb_cancel_all{false};
		sc_uint<8>  wb_cancel_idx{0};
	};
}
