#include <systemc.h>
#include "dcache.h"

thpg::wa824::DCache::DCache(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	for (int i = 0; i < 256; i++) {
		tag_store[i]  = 0;
		data_store[i] = 0;
	}

	SC_METHOD(refresh_read);
	sensitive << reset << addr << mem_read << mem_write << wdata
	          << mmio_enable << mmio_base << mmio_mask << mmio_ordered
	          << fill_done_ << invalidate_event_ << wb_changed_;

	SC_METHOD(refresh_wb);
	sensitive << clk.pos();

	SC_METHOD(refresh_output);
	sensitive << reset << req_we << req_re << req_urgent
	          << req_addr << req_wdata;

	if (tf) {
		std::string mn = std::string(this->name()) + "(DCache)";
		sc_trace(tf, addr,      (mn + ".ADDR").c_str());
		sc_trace(tf, wdata,     (mn + ".WDATA").c_str());
		sc_trace(tf, mem_read,  (mn + ".MEM_READ").c_str());
		sc_trace(tf, mem_write, (mn + ".MEM_WRITE").c_str());
		sc_trace(tf, rdata,     (mn + ".RDATA").c_str());
		sc_trace(tf, hit,       (mn + ".HIT").c_str());
		sc_trace(tf, mmio_enable, (mn + ".MMIO_ENABLE").c_str());
		sc_trace(tf, mmio_base,   (mn + ".MMIO_BASE").c_str());
		sc_trace(tf, mmio_mask,   (mn + ".MMIO_MASK").c_str());
		sc_trace(tf, mmio_ordered,(mn + ".MMIO_ORDERED").c_str());
		sc_trace(tf, dc_addr,   (mn + ".DC_ADDR").c_str());
		sc_trace(tf, dc_wdata,  (mn + ".DC_WDATA").c_str());
		sc_trace(tf, dc_we,     (mn + ".DC_WE").c_str());
		sc_trace(tf, dc_re,     (mn + ".DC_RE").c_str());
		sc_trace(tf, dc_urgent, (mn + ".DC_URGENT").c_str());
		sc_trace(tf, dc_rdata,  (mn + ".DC_RDATA").c_str());
		sc_trace(tf, dc_ready,  (mn + ".DC_READY").c_str());
		sc_trace(tf, dc_grant,  (mn + ".DC_GRANT").c_str());
	}
}

bool thpg::wa824::DCache::is_mmio(uint16_t a) const
{
	if (!mmio_enable.read()) return false;
	if (!mmio_base.read().is_01() || !mmio_mask.read().is_01()) return false;

	const uint16_t base = static_cast<uint16_t>(mmio_base.read().to_uint());
	const uint16_t mask = static_cast<uint16_t>(mmio_mask.read().to_uint());
	return (a & mask) == (base & mask);
}

bool thpg::wa824::DCache::wb_lookup(uint16_t a, sc_uint<8> &data) const
{
	// 가장 최근에 push된 entry부터 찾는다.
	for (unsigned int i = 0; i < wb_count; ++i) {
		const unsigned int idx =
		    (wb_tail + WB_DEPTH - 1u - i) % WB_DEPTH;
		if (wb_q[idx].addr.to_uint() == a) {
			data = wb_q[idx].data;
			return true;
		}
	}
	return false;
}

bool thpg::wa824::DCache::debug_peek_byte(uint16_t a, uint8_t &value) const
{
	if (is_mmio(a)) return false;

	sc_uint<8> queued;
	if (wb_lookup(a, queued)) {
		value = static_cast<uint8_t>(queued.to_uint());
		return true;
	}

	const unsigned int idx = a & 0xFFu;
	const unsigned int tag = (a >> 8) & 0xFFu;
	const bool valid = tag_store[idx][8].to_bool();
	if (valid && (tag_store[idx].range(7, 0) == sc_uint<8>(tag))) {
		value = static_cast<uint8_t>(data_store[idx].to_uint());
		return true;
	}
	return false;
}

void thpg::wa824::DCache::wb_push(uint16_t a, uint8_t data)
{
	if (wb_count >= WB_DEPTH) return;
	wb_q[wb_tail].addr = a;
	wb_q[wb_tail].data = data;
	wb_tail = (wb_tail + 1u) % WB_DEPTH;
	++wb_count;
}

void thpg::wa824::DCache::wb_pop(void)
{
	if (wb_count == 0) return;
	wb_head = (wb_head + 1u) % WB_DEPTH;
	--wb_count;
	if (wb_count == 0) {
		wb_head = 0;
		wb_tail = 0;
	}
}

void thpg::wa824::DCache::wb_remove(uint16_t a)
{
	WriteEntry tmp[WB_DEPTH];
	unsigned int n = 0;

	for (unsigned int i = 0; i < wb_count; ++i) {
		const unsigned int idx = (wb_head + i) % WB_DEPTH;
		if (wb_q[idx].addr.to_uint() != a) {
			tmp[n++] = wb_q[idx];
		}
	}

	for (unsigned int i = 0; i < n; ++i) {
		wb_q[i] = tmp[i];
	}
	wb_head  = 0;
	wb_tail  = (n == WB_DEPTH) ? 0u : n;
	wb_count = n;
}

void thpg::wa824::DCache::wb_clear(void)
{
	wb_head  = 0;
	wb_tail  = 0;
	wb_count = 0;
}

void thpg::wa824::DCache::refresh_read(void)
{
	// 기본값: 요청 없음
	req_we.write(false);
	req_re.write(false);
	req_urgent.write(false);
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

	const bool reading = mem_read.read();
	const bool writing = mem_write.read();
	const unsigned int a = addr.read().to_uint();

	auto start_background_drain = [&]() {
		if (wb_count == 0) return;
		req_we.write(true);
		req_addr.write(wb_q[wb_head].addr);
		req_wdata.write(wb_q[wb_head].data);
	};

	// CPU 요청이 없을 때는 write buffer를 background로 drain한다.
	if (!reading && !writing) {
		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_Z));
		start_background_drain();
		return;
	}

	// ── MMIO: cache/write buffer를 우회하고 direct access ──────
	if (is_mmio(static_cast<uint16_t>(a))) {
		if (mmio_done) {
			hit.write(true);
			rdata.write(reading ? sc_lv<8>(mmio_rdata)
			                    : sc_lv<8>(SC_LOGIC_Z));
			return;
		}

		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_Z));

		// ordered 모드: MMIO 전에 이전 normal store를 모두 drain한다.
		if (mmio_ordered.read() && wb_count > 0) {
			req_we.write(true);
			req_urgent.write(true);
			req_addr.write(wb_q[wb_head].addr);
			req_wdata.write(wb_q[wb_head].data);
			return;
		}

		req_urgent.write(true);
		req_addr.write(sc_lv<16>(a));
		if (reading) {
			req_re.write(true);
		} else {
			req_we.write(true);
			req_wdata.write(wdata.read());
		}
		return;
	}

	const unsigned int idx = a & 0xFFu;
	const unsigned int tag = (a >> 8) & 0xFFu;
	const bool valid   = tag_store[idx][8].to_bool();
	const bool tag_hit = valid && (tag_store[idx].range(7, 0) == sc_uint<8>(tag));

	if (reading) {
		if (tag_hit) {
			hit.write(true);
			rdata.write(sc_lv<8>(data_store[idx]));
			start_background_drain();
			return;
		}

		// store-to-load forwarding: 아직 memory로 나가지 않은 store가 있으면
		// cache 대신 write buffer의 최신 값을 반환한다.
		sc_uint<8> fwd;
		if (wb_lookup(static_cast<uint16_t>(a), fwd)) {
			hit.write(true);
			rdata.write(sc_lv<8>(fwd));
			start_background_drain();
			return;
		}

		hit.write(false);
		rdata.write(sc_lv<8>(SC_LOGIC_X));
		req_re.write(true);
		req_addr.write(sc_lv<16>(a));
		return;
	}

	// ── normal store: queue 공간이 있으면 즉시 accept ──────────
	hit.write(wb_has_space() && wdata.read().is_01());
	rdata.write(sc_lv<8>(SC_LOGIC_Z));

	// store와 무관하게 buffer가 남아 있으면 background drain을 시작한다.
	start_background_drain();
}

void thpg::wa824::DCache::refresh_wb(void)
{
	// 동기 리셋
	if (reset.read()) {
		for (int i = 0; i < 256; i++) {
			tag_store[i]  = 0;
			data_store[i] = 0;
		}
		wb_clear();
		mmio_done  = false;
		mmio_rdata = 0;
		return;
	}

	// MMIO 응답은 hit=1로 한 사이클 노출한 뒤 다음 클럭에서 소비한다.
	if (mmio_done) {
		mmio_done = false;
		wb_changed_.notify(SC_ZERO_TIME);
		return;
	}

	// queue 공간은 bus 완료로 entry가 빠지기 전 상태로 판단한다.
	// 그래야 queue가 가득 찬 store가 같은 엣지에서 중복 enqueue되지 않는다.
	const bool addr_ok = addr.read().is_01() && wdata.read().is_01();
	const bool cur_mmio = addr_ok && is_mmio(static_cast<uint16_t>(addr.read().to_uint()));
	const bool store_accept = mem_write.read() && addr_ok && !cur_mmio &&
	                          wb_has_space();

	// ── bus 응답 처리 ─────────────────────────────────────────
	if (dc_grant.read() && dc_ready.read() &&
	    (req_re.read() || req_we.read()) && req_addr.read().is_01()) {
		const uint16_t ra = static_cast<uint16_t>(req_addr.read().to_uint());
		const bool req_is_mmio = is_mmio(ra);

		if (req_re.read()) {
			if (!dc_rdata.read().is_01()) {
				// 아직 데이터가 유효하지 않으면 다음 클럭에 다시 시도한다.
			} else if (req_is_mmio) {
				mmio_rdata = dc_rdata.read().to_uint();
				mmio_done  = true;
				wb_changed_.notify(SC_ZERO_TIME);
			} else {
				const unsigned int idx = ra & 0xFFu;
				const unsigned int tag = (ra >> 8) & 0xFFu;
				data_store[idx] = dc_rdata.read().to_uint();
				tag_store[idx]  = (1 << 8) | tag;
				fill_done_.notify(SC_ZERO_TIME);
			}
		} else { // req_we
			if (req_is_mmio) {
				mmio_done = true;
				wb_changed_.notify(SC_ZERO_TIME);
			} else {
				wb_pop();
				wb_changed_.notify(SC_ZERO_TIME);
			}
		}
	}

	// ── normal store enqueue + write-through cache update ─────
	if (store_accept) {
		const uint16_t a = static_cast<uint16_t>(addr.read().to_uint());
		const uint8_t  d = static_cast<uint8_t>(wdata.read().to_uint());

		const unsigned int idx = a & 0xFFu;
		const unsigned int tag = (a >> 8) & 0xFFu;
		const bool valid = tag_store[idx][8].to_bool();
		if (valid && (tag_store[idx].range(7, 0) == sc_uint<8>(tag))) {
			data_store[idx] = d;
		}

		wb_push(a, d);
		wb_changed_.notify(SC_ZERO_TIME);
	}
}

void thpg::wa824::DCache::refresh_output(void)
{
	dc_addr.write(req_addr.read());
	dc_wdata.write(req_wdata.read());
	dc_we.write(req_we.read());
	dc_re.write(req_re.read());
	dc_urgent.write(req_urgent.read());
}

void thpg::wa824::DCache::invalidate_line(uint16_t addr)
{
	const unsigned int idx = addr & 0xFFu;

	tag_store[idx]  = 0;
	data_store[idx] = 0;

	wb_remove(addr);
	wb_changed_.notify(SC_ZERO_TIME);
	invalidate_event_.notify(SC_ZERO_TIME);
}

void thpg::wa824::DCache::invalidate_all(void)
{
	for (int i = 0; i < 256; ++i) {
		tag_store[i]  = 0;
		data_store[i] = 0;
	}

	wb_clear();
	mmio_done = false;
	wb_changed_.notify(SC_ZERO_TIME);
	invalidate_event_.notify(SC_ZERO_TIME);
}
