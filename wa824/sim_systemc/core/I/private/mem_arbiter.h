#pragma once
#include <systemc.h>

// Memory Arbiter
// I-Cache와 D-Cache의 메인 메모리 접근 충돌 중재
// 우선순위: D-Cache > I-Cache
//
// 메인 메모리 인터페이스 (외부):
//   mem_addr, mem_wdata, mem_we, mem_re → 메인 메모리
//   mem_rdata, mem_ready                ← 메인 메모리
//
// I-Cache 인터페이스 (내부):
//   ic_addr, ic_re   → arbiter
//   ic_rdata, ic_ready, ic_valid ← arbiter
//
// D-Cache 인터페이스 (내부):
//   dc_addr, dc_wdata, dc_we, dc_re → arbiter
//   dc_rdata, dc_ready, dc_valid    ← arbiter
//
// 동작:
//   D-Cache 요청 있으면 우선 처리, I-Cache는 grant=0으로 대기
//   D-Cache 요청 없으면 I-Cache 처리
//   mem_ready=0이면 현재 요청자에게 ready=0 전달 (wait 상태)
//   grant 변경 시 이전 트랜잭션 완료 후 전환 (mem_ready 기준)

namespace thpg::wa824 {
	class MemArbiter : public sc_module {
	public:
		// ── 메인 메모리 인터페이스 (외부 포트) ──────────────
		sc_out<sc_lv<16>> mem_addr {"MEM_ADDR"};
		sc_out<sc_lv<8>>  mem_wdata{"MEM_WDATA"};
		sc_out<bool>      mem_we   {"MEM_WE"};
		sc_out<bool>      mem_re   {"MEM_RE"};
		sc_in<sc_lv<8>>   mem_rdata{"MEM_RDATA"};
		sc_in<bool>       mem_ready{"MEM_READY"}; // 1=데이터 유효/쓰기 완료, 0=대기

		// ── I-Cache 인터페이스 ────────────────────────────────
		sc_in<sc_lv<16>>  ic_addr  {"IC_ADDR"};
		sc_in<bool>       ic_re    {"IC_RE"};     // I-Cache가 read 요청
		sc_out<sc_lv<8>>  ic_rdata {"IC_RDATA"};
		sc_out<bool>      ic_ready {"IC_READY"};  // 1=ic_rdata 유효
		sc_out<bool>      ic_grant {"IC_GRANT"};  // 1=버스 사용 권한 있음

		// ── D-Cache 인터페이스 ────────────────────────────────
		sc_in<sc_lv<16>>  dc_addr  {"DC_ADDR"};
		sc_in<sc_lv<8>>   dc_wdata {"DC_WDATA"};
		sc_in<bool>       dc_we    {"DC_WE"};     // D-Cache write 요청
		sc_in<bool>       dc_re    {"DC_RE"};     // D-Cache read 요청
		sc_out<sc_lv<8>>  dc_rdata {"DC_RDATA"};
		sc_out<bool>      dc_ready {"DC_READY"};  // 1=dc_rdata 유효 또는 write 완료
		sc_out<bool>      dc_grant {"DC_GRANT"};  // 1=버스 사용 권한 있음

		MemArbiter(sc_module_name name, sc_trace_file *tf = nullptr);

		// combinational: 버스 중재 및 신호 라우팅
		void refresh(void);
	};
}
