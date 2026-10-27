#include <systemc.h>
#include "mem_arbiter.h"

thpg::wa824::MemArbiter::MemArbiter(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive
		<< ic_addr << ic_re
		<< dc_addr << dc_wdata << dc_we << dc_re
		<< mem_rdata << mem_ready;

	if (tf) {
		std::string mn = std::string(this->name()) + "(MemArbiter)";
		sc_trace(tf, mem_addr,  (mn + ".MEM_ADDR").c_str());
		sc_trace(tf, mem_wdata, (mn + ".MEM_WDATA").c_str());
		sc_trace(tf, mem_we,    (mn + ".MEM_WE").c_str());
		sc_trace(tf, mem_re,    (mn + ".MEM_RE").c_str());
		sc_trace(tf, mem_rdata, (mn + ".MEM_RDATA").c_str());
		sc_trace(tf, mem_ready, (mn + ".MEM_READY").c_str());
		sc_trace(tf, ic_addr,   (mn + ".IC_ADDR").c_str());
		sc_trace(tf, ic_re,     (mn + ".IC_RE").c_str());
		sc_trace(tf, ic_rdata,  (mn + ".IC_RDATA").c_str());
		sc_trace(tf, ic_ready,  (mn + ".IC_READY").c_str());
		sc_trace(tf, ic_grant,  (mn + ".IC_GRANT").c_str());
		sc_trace(tf, dc_addr,   (mn + ".DC_ADDR").c_str());
		sc_trace(tf, dc_wdata,  (mn + ".DC_WDATA").c_str());
		sc_trace(tf, dc_we,     (mn + ".DC_WE").c_str());
		sc_trace(tf, dc_re,     (mn + ".DC_RE").c_str());
		sc_trace(tf, dc_rdata,  (mn + ".DC_RDATA").c_str());
		sc_trace(tf, dc_ready,  (mn + ".DC_READY").c_str());
		sc_trace(tf, dc_grant,  (mn + ".DC_GRANT").c_str());
	}
}

void thpg::wa824::MemArbiter::refresh(void)
{
	bool dc_wants = dc_re.read() || dc_we.read();
	bool ic_wants = ic_re.read();

	// I-Cache 버스트 락: 한 번 I-Cache에 grant하면 ic_re가 내려갈 때까지
	// D-Cache가 새치기하지 못하게 한다.
	if (!ic_wants) ic_busy_ = false;
	bool grant_ic = ic_wants && (ic_busy_ || !dc_wants);

	if (grant_ic) {
		// ── I-Cache 처리 ─────────────────────────────────────
		ic_busy_ = true;
		ic_grant.write(true);
		dc_grant.write(false);

		mem_addr.write(ic_addr.read());
		mem_wdata.write(sc_lv<8>(SC_LOGIC_Z));
		mem_we.write(false);
		mem_re.write(true);

		// I-Cache에 메모리 응답 전달
		// I-Cache는 24bit 명령어를 3번 읽어야 하므로
		// rdata는 8bit씩 전달, ic 쪽에서 3바이트 조합
		ic_rdata.write(mem_rdata.read());
		ic_ready.write(mem_ready.read());

		// D-Cache: 요청 없음
		dc_rdata.write(sc_lv<8>(SC_LOGIC_Z));
		dc_ready.write(false);

	} else if (dc_wants) {
		// ── D-Cache 우선 ─────────────────────────────────────
		dc_grant.write(true);
		ic_grant.write(false);

		mem_addr.write(dc_addr.read());
		mem_wdata.write(dc_wdata.read());
		mem_we.write(dc_we.read());
		mem_re.write(dc_re.read());

		// D-Cache에 메모리 응답 전달
		dc_rdata.write(mem_rdata.read());
		dc_ready.write(mem_ready.read());

		// I-Cache: 대기 (ready=0, rdata=X)
		ic_rdata.write(sc_lv<8>(SC_LOGIC_Z));
		ic_ready.write(false);

	} else {
		// ── 요청 없음 ─────────────────────────────────────────
		ic_grant.write(false);
		dc_grant.write(false);

		mem_addr.write(sc_lv<16>(SC_LOGIC_Z));
		mem_wdata.write(sc_lv<8>(SC_LOGIC_Z));
		mem_we.write(false);
		mem_re.write(false);

		ic_rdata.write(sc_lv<8>(SC_LOGIC_Z));
		ic_ready.write(false);
		dc_rdata.write(sc_lv<8>(SC_LOGIC_Z));
		dc_ready.write(false);
	}
}
