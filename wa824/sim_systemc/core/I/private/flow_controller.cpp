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

	// B-Type offset: {inst[19:16], inst[15:12]}
	sc_lv<8> b;
	b.range(3, 0) = inst.read().range(15, 12);
	b.range(7, 4) = inst.read().range(19, 16);
	b_offset.write(b);

	// JR/JA-Type imme: {inst[23:16], inst[15:8]} — 동일 비트, 별도 출력
	sc_lv<16> jrimm;
	jrimm.range(7,  0) = inst.read().range(15, 8);
	jrimm.range(15, 8) = inst.read().range(23, 16);
	jr_imme.write(jrimm);
	ja_imme.write(jrimm);
}

void thpg::wa824::FlowController::refresh_jump_type(void)
{
	// JALR(is_jalr=1): JRP와 동일 경로 (R[WRS]+imme) → jump_type=10
	if (is_jalr.read()) {
		jump_type.write(sc_lv<2>("10"));
		return;
	}

	if (!inst.read().is_01()) {
		jump_type.write(sc_lv<2>(SC_LOGIC_X));
		return;
	}

	unsigned int opcode = inst.read().range(3, 0).to_uint();

	switch (opcode) {
		case 0x4: jump_type.write(sc_lv<2>("00")); break; // branch or normal
		case 0x5: jump_type.write(sc_lv<2>("11")); break; // JA (absolute)
		case 0x6: jump_type.write(sc_lv<2>("01")); break; // JR (relative)
		case 0x7: jump_type.write(sc_lv<2>("10")); break; // JRP
		default:  jump_type.write(sc_lv<2>("00")); break; // 순차
	}
}
