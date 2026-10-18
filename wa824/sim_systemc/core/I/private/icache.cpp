#include <systemc.h>
#include "icache.h"

thpg::wa824::ICache::ICache(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	for (int i = 0; i < 128; i++) { tag_even[i] = 0; tag_odd[i] = 0; }
	for (int i = 0; i < 256; i++) { data_b0[i] = 0; data_b1[i] = 0; data_b2[i] = 0; }
	fill_state = 0;
	fill_addr  = 0;

	SC_METHOD(refresh_hit);
	sensitive << pc;

	SC_METHOD(refresh_fill);
	sensitive << clk.pos();

	if (tf) {
		std::string mn = std::string(this->name()) + "(ICache)";
		sc_trace(tf, pc,       (mn + ".PC").c_str());
		sc_trace(tf, inst,     (mn + ".INST").c_str());
		sc_trace(tf, hit,      (mn + ".HIT").c_str());
		sc_trace(tf, ic_addr,  (mn + ".IC_ADDR").c_str());
		sc_trace(tf, ic_re,    (mn + ".IC_RE").c_str());
		sc_trace(tf, ic_rdata, (mn + ".IC_RDATA").c_str());
		sc_trace(tf, ic_ready, (mn + ".IC_READY").c_str());
		sc_trace(tf, ic_grant, (mn + ".IC_GRANT").c_str());
	}
}

void thpg::wa824::ICache::refresh_hit(void)
{
	if (!pc.read().is_01()) {
		hit.write(false);
		inst.write(sc_lv<24>(SC_LOGIC_X));
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
		return;
	}

	unsigned int addr   = pc.read().to_uint();
	unsigned int index  = addr & 0xFF;
	unsigned int tag    = (addr >> 8) & 0xFF;
	unsigned int offset = addr % 3;

	auto get_tag = [&](unsigned int idx) -> sc_uint<9> {
		return (idx & 1) ? tag_odd[idx >> 1] : tag_even[idx >> 1];
	};

	unsigned int index1 = (index + 1) & 0xFF;
	sc_uint<9> te = get_tag(index);
	bool hit0 = te[8].to_bool() && (te.range(7,0) == sc_uint<8>(tag));

	if (offset == 0) {
		if (!hit0) {
			hit.write(false);
			inst.write(sc_lv<24>(SC_LOGIC_X));
			ic_re.write(true);
			ic_addr.write(sc_lv<16>(addr));
			return;
		}
		sc_lv<24> out;
		out.range(7,  0) = sc_lv<8>(data_b0[index]);
		out.range(15, 8) = sc_lv<8>(data_b1[index]);
		out.range(23,16) = sc_lv<8>(data_b2[index]);
		inst.write(out);
		hit.write(true);
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
	} else {
		sc_uint<9> te1 = get_tag(index1);
		bool hit1 = te1[8].to_bool() && (te1.range(7,0) == sc_uint<8>(tag));

		if (!hit0 || !hit1) {
			hit.write(false);
			inst.write(sc_lv<24>(SC_LOGIC_X));
			ic_re.write(true);
			ic_addr.write(!hit0 ? sc_lv<16>(addr) : sc_lv<16>((addr & 0xFF00) | index1));
			return;
		}

		sc_lv<24> out;
		if (offset == 1) {
			out.range(7,  0) = sc_lv<8>(data_b1[index]);
			out.range(15, 8) = sc_lv<8>(data_b2[index]);
			out.range(23,16) = sc_lv<8>(data_b0[index1]);
		} else {
			out.range(7,  0) = sc_lv<8>(data_b2[index]);
			out.range(15, 8) = sc_lv<8>(data_b0[index1]);
			out.range(23,16) = sc_lv<8>(data_b1[index1]);
		}
		inst.write(out);
		hit.write(true);
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
	}
}

void thpg::wa824::ICache::refresh_fill(void)
{
	if (reset.read()) {
		for (int i = 0; i < 128; i++) { tag_even[i] = 0; tag_odd[i] = 0; }
		for (int i = 0; i < 256; i++) { data_b0[i] = 0; data_b1[i] = 0; data_b2[i] = 0; }
		fill_state = 0;
		return;
	}

	// grant 없으면 대기
	if (!ic_grant.read()) return;
	if (!ic_ready.read()) return;
	if (!ic_rdata.read().is_01()) return;
	if (!pc.read().is_01()) return;

	unsigned int addr  = pc.read().to_uint();
	unsigned int index = addr & 0xFF;
	unsigned int tag   = (addr >> 8) & 0xFF;

	// fill_state 순서대로 3바이트 수신
	switch (fill_state.to_uint()) {
		case 0: // 첫 바이트 요청 시작 → 바로 수신
			fill_addr  = addr;
			fill_buf[0] = ic_rdata.read().to_uint();
			fill_state = 1;
			break;
		case 1:
			fill_buf[1] = ic_rdata.read().to_uint();
			fill_state = 2;
			break;
		case 2:
			fill_buf[2] = ic_rdata.read().to_uint();
			// 3바이트 모두 수신 → 캐시에 저장
			{
				unsigned int fidx = fill_addr.to_uint() & 0xFF;
				unsigned int ftag = (fill_addr.to_uint() >> 8) & 0xFF;
				data_b0[fidx] = fill_buf[0];
				data_b1[fidx] = fill_buf[1];
				data_b2[fidx] = fill_buf[2];
				sc_uint<9> entry = (1 << 8) | ftag;
				if (fidx & 1) tag_odd[fidx >> 1]  = entry;
				else          tag_even[fidx >> 1] = entry;
			}
			fill_state = 0;
			break;
	}
}
