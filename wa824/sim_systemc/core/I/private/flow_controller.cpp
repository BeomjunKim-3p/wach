#include <systemc.h>
#include "flow_controller.h"

thpg::wa824::FlowController::FlowController(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh_imme);
	sensitive << inst;

	SC_METHOD(refresh_jump_type);
	sensitive << inst << branch_pred << is_jalr;

	if (tf) {
		std::string mn = std::string(this->name()) + "(FlowController)";
		sc_trace(tf, inst,        (mn + ".INST").c_str());
		sc_trace(tf, branch_pred, (mn + ".BRANCH_PRED").c_str());
		sc_trace(tf, is_jalr,     (mn + ".IS_JALR").c_str());
		sc_trace(tf, b_offset,    (mn + ".B_OFFSET").c_str());
		sc_trace(tf, jr_imme,     (mn + ".JR_IMME").c_str());
		sc_trace(tf, ja_imme,     (mn + ".JA_IMME").c_str());
		sc_trace(tf, jump_type,   (mn + ".JUMP_TYPE").c_str());
	}
}

void thpg::wa824::FlowController::refresh_imme(void)
{
	if (!inst.read().is_01()) {
		b_offset.write(sc_lv<8>(SC_LOGIC_X));
		jr_imme.write(sc_lv<16>(SC_LOGIC_X));
		ja_imme.write(sc_lv<16>(SC_LOGIC_X));
		return;
	}


	// B-Type offset: {inst[23:20], inst[11:8]}
	b_offset.write(sc_lv<8>((((inst.read().to_uint() >> 20) & 0x0F) << 4) | ((inst.read().to_uint() >> 8) & 0x0F)));

	// JR/JA-Type imme: {inst[23:16], inst[15:8]} — 동일 비트, 별도 출력
	ja_imme.write(sc_lv<16>(((inst.read() >> 16) & 0xFF) << 8 | ((inst.read() >> 8) & 0xFF)));
	jr_imme.write(sc_lv<16>(((inst.read() >> 16) & 0xFF) << 8 | ((inst.read() >> 8) & 0xFF)));
}

void thpg::wa824::FlowController::refresh_jump_type(void)
{
	// JRP/JALR(is_jalr=1): ID단에서 R[WRS]+imme 경로 선택 → jump_type=10
	// is_jalr은 IF/ID의 opcode 0x7에서 조합적으로 생성된다.
	if (is_jalr.read()) {
		jump_type.write(sc_lv<2>("10"));
		return;
	}

	if (!inst.read().is_01()) {
		jump_type.write(sc_lv<2>(SC_LOGIC_X));
		return;
	}

	unsigned int opcode = ((inst.read().to_uint() >> 4) & 0x0F);

	switch (opcode) {
		case 0x4: jump_type.write(sc_lv<2>("00")); break; // branch or normal
		case 0x5: jump_type.write(sc_lv<2>("11")); break; // JA (absolute)
		case 0x6: jump_type.write(sc_lv<2>("01")); break; // JR (relative)
		// JRP/JALR(0x7)은 RF 의존이라 IF에서 jump_type=10을 만들지 않는다.
		// ID의 is_jalr 입력이 10을 선택한다.
		default:  jump_type.write(sc_lv<2>("00")); break; // 순차
	}
}
