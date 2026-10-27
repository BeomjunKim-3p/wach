#include <systemc.h>
#include "dcache.h"

thpg::wa824::DCache::DCache(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	for (int i = 0; i < 256; i++) {
		tag_store[i]  = 0;
		dirty[i]      = false;
		data_store[i] = 0;
	}
	wb_pending  = false;
	wb_addr_buf = 0;
	wb_data_buf = 0;

	SC_METHOD(refresh_read);
	sensitive << reset << addr << mem_read << mem_write << wdata << fill_done_
	          << invalidate_event_;

	SC_METHOD(refresh_wb);
	sensitive << clk.pos();

	SC_METHOD(refresh_output);
	sensitive << reset << req_we << req_re << req_addr << req_wdata
	          << wb_active << wb_we << wb_re << wb_addr << wb_wdata
	          << invalidate_event_;

	if (tf) {
		std::string mn = std::string(this->name()) + "(DCache)";
		sc_trace(tf, addr,      (mn + ".ADDR").c_str());
		sc_trace(tf, wdata,     (mn + ".WDATA").c_str());
		sc_trace(tf, mem_read,  (mn + ".MEM_READ").c_str());
		sc_trace(tf, mem_write, (mn + ".MEM_WRITE").c_str());
		sc_trace(tf, rdata,     (mn + ".RDATA").c_str());
		sc_trace(tf, hit,       (mn + ".HIT").c_str());
		sc_trace(tf, dc_addr,   (mn + ".DC_ADDR").c_str());
		sc_trace(tf, dc_wdata,  (mn + ".DC_WDATA").c_str());
		sc_trace(tf, dc_we,     (mn + ".DC_WE").c_str());
		sc_trace(tf, dc_re,     (mn + ".DC_RE").c_str());
		sc_trace(tf, dc_rdata,  (mn + ".DC_RDATA").c_str());
		sc_trace(tf, dc_ready,  (mn + ".DC_READY").c_str());
		sc_trace(tf, dc_grant,  (mn + ".DC_GRANT").c_str());
	}
}

void thpg::wa824::DCache::refresh_read(void)
{
	// 기본값: 요청 없음
	req_we.write(false);
	req_re.write(false);
	req_addr.write(sc_lv<16>(SC_LOGIC_Z));
	req_wdata.write(sc_lv<8>(SC_LOGIC_Z));

	if (reset.read()) {
		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_Z));
		return;
	}

	if (!addr.read().is_01()) {
		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_X));
		return;
	}

	if (!mem_read.read() && !mem_write.read()) {
		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_Z));
		return;
	}

	unsigned int a   = addr.read().to_uint();
	unsigned int idx = a & 0xFF;
	unsigned int tag = (a >> 8) & 0xFF;

	bool valid   = tag_store[idx][8].to_bool();
	bool tag_hit = valid && (tag_store[idx].range(7, 0) == sc_uint<8>(tag));

	if (mem_read.read()) {
		if (tag_hit) {
			hit.write(true);
			rdata.write(sc_lv<8>(data_store[idx]));
		} else {
			hit.write(false);
			rdata.write(sc_lv<8>(SC_LOGIC_X));
			// WB가 진행 중이면 refresh_output이 wb_*를 우선한다.
			req_re.write(true);
			req_addr.write(sc_lv<16>(a));
		}
		return;
	}

	if (mem_write.read()) {
		if (tag_hit) {
			// write hit: 조합논리에서는 hit 표시만, 실제 쓰기는 tick에서
			hit.write(true);
			rdata.write(sc_lv<8>(SC_LOGIC_Z));
		} else {
			// write miss: write-allocate → fetch 필요
			hit.write(false);
			rdata.write(sc_lv<8>(SC_LOGIC_Z));
			req_re.write(true);
			req_addr.write(sc_lv<16>(a));
		}
	}
}

void thpg::wa824::DCache::refresh_wb(void)
{
	// 동기 리셋
	if (reset.read()) {
		for (int i = 0; i < 256; i++) {
			tag_store[i]  = 0;
			dirty[i]      = false;
			data_store[i] = 0;
		}
		wb_pending = false;
		wb_cancel_valid = false;
		wb_cancel_all   = false;
		wb_active.write(false);
		wb_we.write(false);
		wb_re.write(false);
		wb_addr.write(sc_lv<16>(SC_LOGIC_Z));
		wb_wdata.write(sc_lv<8>(SC_LOGIC_Z));
		return;
	}

	// 디버그 무효화로 취소된 write-back은 메모리에 쓰지 않고 버린다.
	// refresh_output은 invalidate_event_ 직후부터 wb_* 요청을 억제하므로,
	// 다음 클럭 전에 메인 메모리가 이전 값을 덮어쓰는 일도 없다.
	if (wb_cancel_all) {
		wb_pending      = false;
		wb_cancel_all   = false;
		wb_cancel_valid = false;
		wb_active.write(false);
		wb_we.write(false);
		wb_re.write(false);
		wb_addr.write(sc_lv<16>(SC_LOGIC_Z));
		wb_wdata.write(sc_lv<8>(SC_LOGIC_Z));
		return;
	}
	if (wb_cancel_valid) {
		const bool cancelled = wb_pending &&
		    ((wb_addr_buf.to_uint() & 0xFFu) == wb_cancel_idx.to_uint());
		wb_cancel_valid = false;
		if (cancelled) {
			wb_pending = false;
			wb_active.write(false);
			wb_we.write(false);
			wb_re.write(false);
			wb_addr.write(sc_lv<16>(SC_LOGIC_Z));
			wb_wdata.write(sc_lv<8>(SC_LOGIC_Z));
			return;
		}
	}

	if (!addr.read().is_01()) return;

	unsigned int a   = addr.read().to_uint();
	unsigned int idx = a & 0xFF;
	unsigned int tag = (a >> 8) & 0xFF;

	bool valid   = tag_store[idx][8].to_bool();
	bool tag_hit = valid && (tag_store[idx].range(7, 0) == sc_uint<8>(tag));

	// ── write hit: 캐시 갱신 + dirty 표시 ──────────────────
	if (mem_write.read() && tag_hit && wdata.read().is_01()) {
		data_store[idx] = wdata.read().to_uint();
		dirty[idx]      = true;
		return;
	}

	// ── write-back pending 처리 ──────────────────────────────
	if (wb_pending) {
		if (!dc_grant.read()) return; // 아직 중재기 grant 안 됨
		if (!dc_ready.read()) return; // 메모리 아직 처리 중
		// write-back 완료. 다음 요청(fetch)은 refresh_read가 req_re로
		// 계속 만들고 있으므로 wb override를 해제한다.
		wb_pending = false;
		wb_active.write(false);
		wb_we.write(false);
		wb_re.write(false);
		wb_addr.write(sc_lv<16>(SC_LOGIC_Z));
		wb_wdata.write(sc_lv<8>(SC_LOGIC_Z));
		return;
	}

	// WB override가 남아 있지 않도록 기본값으로 되돌린다.
	wb_active.write(false);
	wb_we.write(false);
	wb_re.write(false);
	wb_addr.write(sc_lv<16>(SC_LOGIC_Z));
	wb_wdata.write(sc_lv<8>(SC_LOGIC_Z));

	// ── miss + grant + ready: fill ──────────────────────────
	if (!tag_hit && (mem_read.read() || mem_write.read())) {
		if (!dc_grant.read()) return;
		if (!dc_ready.read()) return;
		if (!dc_rdata.read().is_01()) return;

		// dirty line eviction → write-back 필요
		if (dirty[idx] && valid) {
			unsigned int old_tag = tag_store[idx].range(7, 0).to_uint();
			wb_addr_buf = (old_tag << 8) | idx;
			wb_data_buf = data_store[idx];
			wb_pending  = true;
			dirty[idx]  = false;
			// write-back 발행 (refresh_output이 dc_*를 구동)
			wb_active.write(true);
			wb_we.write(true);
			wb_re.write(false);
			wb_addr.write(sc_lv<16>(wb_addr_buf.to_uint()));
			wb_wdata.write(sc_lv<8>(wb_data_buf));
			return;
		}

		// fill
		data_store[idx] = dc_rdata.read().to_uint();
		tag_store[idx]  = (1 << 8) | tag;
		dirty[idx]      = false;

		// write-allocate: fill 후 write 요청 있으면 캐시에 기록
		if (mem_write.read() && wdata.read().is_01()) {
			data_store[idx] = wdata.read().to_uint();
			dirty[idx]      = true;
		}
		// addr가 그대로여도 refresh_read가 새 tag로 hit을 재평가해야 한다.
		fill_done_.notify(SC_ZERO_TIME);
	}
}

void thpg::wa824::DCache::refresh_output(void)
{
	if (wb_active.read()) {
		// 무효화된 라인의 write-back이면 메모리 요청을 내리지 않는다.
		if (wb_cancel_all ||
		    (wb_cancel_valid &&
		     ((wb_addr_buf.to_uint() & 0xFFu) == wb_cancel_idx.to_uint()))) {
			dc_addr.write(sc_lv<16>(SC_LOGIC_Z));
			dc_wdata.write(sc_lv<8>(SC_LOGIC_Z));
			dc_we.write(false);
			dc_re.write(false);
			return;
		}
		dc_addr.write(wb_addr.read());
		dc_wdata.write(wb_wdata.read());
		dc_we.write(wb_we.read());
		dc_re.write(wb_re.read());
	} else {
		dc_addr.write(req_addr.read());
		dc_wdata.write(req_wdata.read());
		dc_we.write(req_we.read());
		dc_re.write(req_re.read());
	}
}

void thpg::wa824::DCache::invalidate_line(uint16_t addr)
{
	const unsigned int idx = addr & 0xFFu;

	tag_store[idx]  = 0;
	dirty[idx]      = false;
	data_store[idx] = 0;

	if (wb_pending && ((wb_addr_buf.to_uint() & 0xFFu) == idx)) {
		wb_cancel_valid = true;
		wb_cancel_idx   = idx;
	}

	invalidate_event_.notify(SC_ZERO_TIME);
}

void thpg::wa824::DCache::invalidate_all(void)
{
	for (int i = 0; i < 256; ++i) {
		tag_store[i]  = 0;
		dirty[i]      = false;
		data_store[i] = 0;
	}

	if (wb_pending) wb_cancel_all = true;
	wb_cancel_valid = false;

	invalidate_event_.notify(SC_ZERO_TIME);
}
