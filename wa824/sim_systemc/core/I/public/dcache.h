#pragma once
#include <systemc.h>

// D-Cache: 256 lines, 1byte/line, direct-mapped, write-through
//
// 정책:
//   - write-through + no-write-allocate
//   - store는 write buffer(FIFO)에 push하고, queue 공간이 있으면 즉시 hit
//   - load miss는 메모리에서 1바이트 fill
//   - 같은 주소의 pending store는 store-to-load forwarding
//   - MMIO는 cache/write buffer를 우회하고 arbiter로 direct access
//   - mmio_ordered=1이면 MMIO 전에 write buffer를 drain
//
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

		// 외부에서 설정하는 MMIO 영역/정책
		//   is_mmio = mmio_enable && ((addr & mmio_mask) == (mmio_base & mmio_mask))
		sc_in<bool>       mmio_enable {"MMIO_ENABLE"};
		sc_in<sc_lv<16>>  mmio_base   {"MMIO_BASE"};
		sc_in<sc_lv<16>>  mmio_mask   {"MMIO_MASK"};
		sc_in<bool>       mmio_ordered{"MMIO_ORDERED"};

		// MemArbiter 인터페이스
		sc_out<sc_lv<16>> dc_addr   {"DC_ADDR"};
		sc_out<sc_lv<8>>  dc_wdata  {"DC_WDATA"};
		sc_out<bool>      dc_we     {"DC_WE"};
		sc_out<bool>      dc_re     {"DC_RE"};
		sc_out<bool>      dc_urgent {"DC_URGENT"};
		sc_in<sc_lv<8>>   dc_rdata  {"DC_RDATA"};
		sc_in<bool>       dc_ready  {"DC_READY"};
		sc_in<bool>       dc_grant  {"DC_GRANT"};

		DCache(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh_read(void);   // combinational request generator
		void refresh_wb(void);     // sequential (clk.pos())
		void refresh_output(void); // single driver for dc_* outputs

		// 디버거가 메인 메모리를 수정한 뒤 해당 라인/전체를 무효화한다.
		// write buffer에 남은 같은 주소의 store도 함께 제거한다.
		void invalidate_line(uint16_t addr);
		void invalidate_all(void);

		// 디버그용 write buffer 상태
		unsigned int write_buffer_count(void) const { return wb_count; }
		unsigned int write_buffer_depth(void) const { return WB_DEPTH; }

	private:
		static constexpr unsigned int WB_DEPTH = 8;

		struct WriteEntry {
			sc_uint<16> addr{0};
			sc_uint<8>  data{0};
		};

		sc_uint<9> tag_store[256];
		sc_uint<8> data_store[256];

		// refresh_read와 refresh_wb가 dc_* 포트를 직접 구동하면 sc_signal
		// 다중 드라이버가 되므로, 내부 요청을 모은 뒤 refresh_output에서
		// 한 번만 포트를 구동한다.
		sc_signal<bool>      req_we{"REQ_WE"};
		sc_signal<bool>      req_re{"REQ_RE"};
		sc_signal<bool>      req_urgent{"REQ_URGENT"};
		sc_signal<sc_lv<16>> req_addr{"REQ_ADDR"};
		sc_signal<sc_lv<8>>  req_wdata{"REQ_WDATA"};

		// Write buffer FIFO
		WriteEntry wb_q[WB_DEPTH];
		unsigned int wb_head{0};
		unsigned int wb_tail{0};
		unsigned int wb_count{0};

		// MMIO direct access 응답 latch
		bool       mmio_done{false};
		sc_uint<8> mmio_rdata{0};

		sc_event fill_done_;
		sc_event invalidate_event_;
		sc_event wb_changed_;

		bool is_mmio(uint16_t a) const;
		bool wb_has_space(void) const { return wb_count < WB_DEPTH; }
		bool wb_lookup(uint16_t a, sc_uint<8> &data) const;
		void wb_push(uint16_t a, uint8_t data);
		void wb_pop(void);
		void wb_remove(uint16_t a);
		void wb_clear(void);
	};
} // namespace thpg::wa824
