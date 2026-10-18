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
	sensitive << addr << mem_read << mem_write << wdata;

	SC_METHOD(refresh_wb);
	sensitive << clk.pos();

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
	dc_re.write(false);
	dc_we.write(false);
	dc_addr.write(sc_lv<16>(SC_LOGIC_Z));
	dc_wdata.write(sc_lv<8>(SC_LOGIC_Z));

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
			// write-back 대기 중이면 wb 먼저, 아니면 바로 fetch
			if (!wb_pending) {
				dc_re.write(true);
				dc_addr.write(sc_lv<16>(a));
			}
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
			if (!wb_pending) {
				dc_re.write(true);
				dc_addr.write(sc_lv<16>(a));
			}
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
		wb_pending  = false;
		dc_we.write(false);
		dc_re.write(false);
		return;
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
		// write-back 완료
		wb_pending = false;
		dc_we.write(false);
		// 이제 fetch 시작
		dc_re.write(true);
		dc_addr.write(sc_lv<16>(a));
		return;
	}

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
			// write-back 발행
			dc_we.write(true);
			dc_re.write(false);
			dc_addr.write(sc_lv<16>(wb_addr_buf.to_uint()));
			dc_wdata.write(sc_lv<8>(wb_data_buf));
			return;
		}

		// fill
		data_store[idx] = dc_rdata.read().to_uint();
		tag_store[idx]  = (1 << 8) | tag;
		dirty[idx]      = false;
		dc_re.write(false);

		// write-allocate: fill 후 write 요청 있으면 캐시에 기록
		if (mem_write.read() && wdata.read().is_01()) {
			data_store[idx] = wdata.read().to_uint();
			dirty[idx]      = true;
		}
	}
}
