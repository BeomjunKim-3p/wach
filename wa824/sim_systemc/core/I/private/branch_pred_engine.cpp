#include <systemc.h>
#include "branch_pred_engine.h"

thpg::wa824::BPE::BPE(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << inst << pc;

	if (tf) {
		std::string mn = std::string(this->name()) + "(BPE)";
		sc_trace(tf, inst,                (mn + ".INST").c_str());
		sc_trace(tf, pc,                  (mn + ".PC").c_str());
		sc_trace(tf, branch_pred_result,  (mn + ".BRANCH_PRED_RESULT").c_str());
	}
}

void thpg::wa824::BPE::refresh(void)
{
	if (!inst.read().is_01()) {
		branch_pred_result.write(false);
		return;
	}

	unsigned int opcode = inst.read().range(3, 0).to_uint();

	// B-Type 아니면 not-taken
	if (opcode != 0x4) {
		branch_pred_result.write(false);
		return;
	}

	// BTFNT: imm = {inst[19:16], inst[15:12]}
	// 부호비트 = inst[19], imm == 0x00 도 backward 취급
	unsigned int imm = (inst.read().range(19, 16).to_uint() << 4)
	                 |  inst.read().range(15, 12).to_uint();

	bool backward = (inst.read()[19] == SC_LOGIC_1) || (imm == 0x00);
	branch_pred_result.write(backward);
}
