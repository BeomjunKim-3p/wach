#include <systemc.h>
#include "control_unit.h"

thpg::wa824::ControlUnit::ControlUnit(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << inst;

	if (tf) {
		std::string mn = std::string(this->name()) + "(ControlUnit)";
		sc_trace(tf, inst, (mn + ".INST").c_str());
		sc_trace(tf, ctrl, (mn + ".CTRL").c_str());
	}
}

void thpg::wa824::ControlUnit::refresh(void)
{
	if (!inst.read().is_01()) {
		ctrl.write(sc_lv<14>(SC_LOGIC_X));
		return;
	}

	unsigned int opcode = ((inst.read().to_uint() >> 4) & 0x0F);
	unsigned int funct  = ((inst.read().to_uint() >> 16) & 0x0F);

	// 필드별 초기값 (safe default: 모든 write/read 비활성)
	unsigned int addr2_sel = 0;
	unsigned int addr3_sel = 0;
	unsigned int alu_b_sel = 0;
	unsigned int imme_sel  = 0;
	unsigned int alu_funct = 0;
	unsigned int mem_read  = 0;
	unsigned int mem_write = 0;
	unsigned int we4l      = 0;
	unsigned int we4h      = 0;
	unsigned int wb_sel    = 0;  // 00=alu_out, 01=PC+3, 10=mem_out
	bool valid = true;

	switch (opcode) {
		case 0x0: // R-Type: ADD SUB AND OR XOR SLL SRL SRA SLT SLTU
			if (funct > 0x9) { valid = false; break; }
			addr2_sel = 0;
			addr3_sel = 0;
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = funct;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 1;
			we4h      = 0;
			wb_sel    = 0b00;
			break;

		case 0x1: // I-Type: ADDI ANDI ORI XORI SLLI SRLI SRAI SLTI SLTUI (no SUBI)
			if (funct == 0x1 || funct > 0x9) { valid = false; break; }
			addr2_sel = 0;
			addr3_sel = 0;
			alu_b_sel = 1;
			imme_sel  = 0;
			alu_funct = funct;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 1;
			we4h      = 0;
			wb_sel    = 0b00;
			break;

		case 0x2: // L-Type: LB
			addr2_sel = 0;
			addr3_sel = 0;  // WRS = inst[15:13]
			alu_b_sel = 1;
			imme_sel  = 1;  // L&S imme
			alu_funct = 0x0; // ADD
			mem_read  = 1;
			mem_write = 0;
			we4l      = 1;
			we4h      = 0;
			wb_sel    = 0b10;
			break;

		case 0x3: // S-Type: SB
			addr2_sel = 1;  // store data RS = inst[3:0]
			addr3_sel = 0;  // WRS = inst[15:13]
			alu_b_sel = 1;
			imme_sel  = 1;
			alu_funct = 0x0; // ADD
			mem_read  = 0;
			mem_write = 1;
			we4l      = 0;
			we4h      = 0;
			wb_sel    = 0b00;
			break;

		case 0x4: // B-Type: BEQ BNE BLT BLTU
			if (funct > 0x3) { valid = false; break; }
			addr2_sel = 1;  // RS2 = inst[3:0]
			addr3_sel = 0;
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = 0x0;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 0;
			we4h      = 0;
			wb_sel    = 0b00;
			break;

		case 0x5: // JA-Type: JMPA
			addr2_sel = 0;
			addr3_sel = 0;
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = 0x0;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 1;
			we4h      = 1;
			wb_sel    = 0b01;
			break;

		case 0x6: // JR-Type: JAL
			addr2_sel = 0;
			addr3_sel = 0;
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = 0x0;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 1;
			we4h      = 1;
			wb_sel    = 0b01;
			break;

		case 0x7: // JRP-Type: JALR
			addr2_sel = 0;
			addr3_sel = 1;  // WRS = inst[19:17]
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = 0x0;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 1;
			we4h      = 1;
			wb_sel    = 0b01;
			break;

		case 0x8: // SYSTEM: ECALL EBREAK MRET
			addr2_sel = 0;
			addr3_sel = 0;
			alu_b_sel = 0;
			imme_sel  = 0;
			alu_funct = 0x0;
			mem_read  = 0;
			mem_write = 0;
			we4l      = 0;
			we4h      = 0;
			wb_sel    = 0b00;
			break;

		default:  // invalid opcode
			valid = false;
			break;
	}

	// invalid → 위험 신호 전부 0
	if (!valid) {
		mem_read  = 0;
		mem_write = 0;
		we4l      = 0;
		we4h      = 0;
	}

	sc_lv<14> out;
	out[0]            = addr2_sel ? SC_LOGIC_1 : SC_LOGIC_0;
	out[1]            = addr3_sel ? SC_LOGIC_1 : SC_LOGIC_0;
	out[2]            = alu_b_sel ? SC_LOGIC_1 : SC_LOGIC_0;
	out[3]            = imme_sel  ? SC_LOGIC_1 : SC_LOGIC_0;
	out.range(7,  4)  = sc_lv<4>(alu_funct);
	out[8]            = mem_read  ? SC_LOGIC_1 : SC_LOGIC_0;
	out[9]            = mem_write ? SC_LOGIC_1 : SC_LOGIC_0;
	out[10]           = we4l      ? SC_LOGIC_1 : SC_LOGIC_0;
	out[11]           = we4h      ? SC_LOGIC_1 : SC_LOGIC_0;
	out.range(13, 12) = sc_lv<2>(wb_sel);
	ctrl.write(out);
}
