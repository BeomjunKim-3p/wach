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
	sensitive << pc << fill_done_ << reset
	          << invalidate_event_ << fill_restart_done_;

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
	// reset 중에는 이전 hit/inst가 IF/ID로 전달되지 않도록 즉시 무효화한다.
	// (reset으로 tag를 지운 뒤 refresh_hit이 다시 평가되지 않으면
	//  같은 PC의 stale hit가 남을 수 있다.)
	if (reset.read()) {
		fill_restart = false;
		hit.write(false);
		inst.write(sc_lv<24>(SC_LOGIC_X));
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
		return;
	}

	// 디버그 무효화 직후에는 요청을 한 delta 동안 내려서 MemArbiter의
	// 버스트를 끊고, 다음 delta에서 새 주소로 다시 요청한다. 그렇지 않으면
	// 진행 중이던 fill이 이전 메모리 바이트를 그대로 캐시에 저장할 수 있다.
	if (fill_restart) {
		fill_restart = false;
		hit.write(false);
		inst.write(sc_lv<24>(SC_LOGIC_X));
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
		fill_restart_done_.notify(SC_ZERO_TIME);
		return;
	}

	if (!pc.read().is_01()) {
		hit.write(false);
		inst.write(sc_lv<24>(SC_LOGIC_X));
		ic_re.write(false);
		ic_addr.write(sc_lv<16>(SC_LOGIC_Z));
		return;
	}

	unsigned int addr   = pc.read().to_uint();
	unsigned int offset = addr % 3;
	unsigned int base   = addr - offset;   // 3바이트 라인의 시작 주소
	unsigned int line   = base / 3;        // 256-line direct-mapped index
	unsigned int index  = line & 0xFF;
	unsigned int tag    = (line >> 8) & 0xFF;

	auto get_tag = [&](unsigned int idx) -> sc_uint<9> {
		return (idx & 1) ? tag_odd[idx >> 1] : tag_even[idx >> 1];
	};

	unsigned int index1 = (index + 1) & 0xFF;
	unsigned int tag1   = ((line + 1) >> 8) & 0xFF;
	sc_uint<9> te = get_tag(index);
	bool hit0 = te[8].to_bool() && (te.range(7,0) == sc_uint<8>(tag));

	if (offset == 0) {
		if (!hit0) {
			hit.write(false);
			inst.write(sc_lv<24>(SC_LOGIC_X));
			ic_re.write(true);
			ic_addr.write(sc_lv<16>(base));
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
		bool hit1 = te1[8].to_bool() && (te1.range(7,0) == sc_uint<8>(tag1));

		if (!hit0 || !hit1) {
			hit.write(false);
			inst.write(sc_lv<24>(SC_LOGIC_X));
			ic_re.write(true);
			ic_addr.write(!hit0 ? sc_lv<16>(base)
			                    : sc_lv<16>((base + 3) & 0xFFFFu));
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
		fill_addr  = 0;
		fill_restart = false;
		return;
	}

	// fill 중 PC가 다른 라인으로 바뀌면 이전 버스트를 버리고 새 라인을
	// 처음부터 다시 요청한다. (branch/jump로 fetch 대상이 바뀐 경우)
	// unaligned PC는 현재 라인과 다음 라인을 모두 사용할 수 있으므로
	// 두 라인 중 하나를 채우는 중이면 계속 진행한다.
	if (!pc.read().is_01()) {
		if (fill_state.to_uint() != 0) {
			fill_state = 0;
			fill_restart = true;
			invalidate_event_.notify(SC_ZERO_TIME);
		}
		return;
	}

	const unsigned int pc_addr = pc.read().to_uint();
	const unsigned int pc_base = pc_addr - (pc_addr % 3);
	const unsigned int next_base = (pc_base + 3) & 0xFFFFu;
	if (fill_state.to_uint() != 0 && fill_addr.to_uint() != pc_base &&
	    fill_addr.to_uint() != next_base) {
		fill_state = 0;
		fill_restart = true;
		invalidate_event_.notify(SC_ZERO_TIME);
		return;
	}

	// grant/ready가 중간에 끊기면 메인 메모리의 버스트도 처음부터 다시
	// 시작되므로, fill 상태를 0으로 되돌려 첫 바이트부터 다시 받는다.
	// (D-Cache가 I-Cache fill 도중 버스를 선점하는 경우)
	if (!ic_grant.read() || !ic_ready.read() || !ic_re.read() ||
	    !ic_addr.read().is_01() || !ic_rdata.read().is_01()) {
		if (fill_state.to_uint() != 0) fill_state = 0;
		return;
	}

	const unsigned int request_addr = ic_addr.read().to_uint();

	// fill 도중 PC/버스트 주소가 바뀌면 이전 조각을 버리고 새 주소의
	// 첫 바이트부터 다시 받는다.
	if (fill_state.to_uint() != 0 && request_addr != fill_addr.to_uint()) {
		fill_state = 0;
	}

	// fill_state 순서대로 3바이트 수신
	switch (fill_state.to_uint()) {
		case 0: // 첫 바이트 요청 시작 → 바로 수신
			fill_addr  = request_addr;
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
				unsigned int line = fill_addr.to_uint() / 3;
				unsigned int fidx = line & 0xFF;
				unsigned int ftag = (line >> 8) & 0xFF;
				data_b0[fidx] = fill_buf[0];
				data_b1[fidx] = fill_buf[1];
				data_b2[fidx] = fill_buf[2];
				sc_uint<9> entry = (1 << 8) | ftag;
				if (fidx & 1) tag_odd[fidx >> 1]  = entry;
				else          tag_even[fidx >> 1] = entry;
			}
			fill_state = 0;
			// 요청 주소가 그대로여도 방금 채운 라인을 다시 조회해야 한다.
			fill_done_.notify(SC_ZERO_TIME);
			break;
	}
}

void thpg::wa824::ICache::invalidate_line(uint16_t addr)
{
	// 라인은 3바이트 정렬된 시작 주소를 기준으로 b0/b1/b2를 저장한다.
	// 수정된 바이트를 포함할 수 있는 명령어는 최대 두 바이트 앞에서
	// 시작할 수 있으므로 addr-2..addr가 속한 라인을 모두 무효화한다.
	// (16비트 주소 wrap도 함께 처리한다.)
	for (unsigned int delta = 0; delta < 3; ++delta) {
		const unsigned int byte_addr =
		    (static_cast<unsigned int>(addr) - delta) & 0xFFFFu;
		const unsigned int line = byte_addr / 3u;
		const unsigned int idx  = line & 0xFFu;
		if (idx & 1u) tag_odd[idx >> 1] = 0;
		else          tag_even[idx >> 1] = 0;
	}

	// 진행 중인 fill이 있었다면 버리고, 다음 delta에서 새로 시작한다.
	fill_state  = 0;
	fill_restart = true;
	invalidate_event_.notify(SC_ZERO_TIME);
}

void thpg::wa824::ICache::invalidate_all(void)
{
	for (int i = 0; i < 128; ++i) {
		tag_even[i] = 0;
		tag_odd[i]  = 0;
	}

	fill_state  = 0;
	fill_restart = true;
	invalidate_event_.notify(SC_ZERO_TIME);
}
