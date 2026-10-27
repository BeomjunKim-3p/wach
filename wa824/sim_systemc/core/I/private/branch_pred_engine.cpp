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

	unsigned int opcode = ((inst.read().to_uint() >> 4) & 0x0F);

	// B-Type 아니면 not-taken
	if (opcode != 0x4) {
		branch_pred_result.write(false);
		return;
	}

	unsigned int imm = (((inst.read().to_uint() >> 20) & 0x0F) << 4) |
	                   ((inst.read().to_uint() >> 8) & 0x0F);

	// imm[7] == inst[23] (부호 비트)
	bool backward = ((imm & 0x80u) != 0u) || (imm == 0x00u);
	branch_pred_result.write(backward);
}
