#pragma once
#include <systemc.h>

// ISA 비트 필드 (LSB=bit0, 24bit inst, docs/core/I/ISA.ko-KR.MD):
// R-Type:   [3:0]=rd      [7:4]=opcode  [11:8]=rs2     [15:12]=rs1     [19:16]=funct   [23:20]=unused
// I-Type:   [3:0]=rd      [7:4]=opcode  [11:8]=imm[3:0] [15:12]=rs     [19:16]=funct   [23:20]=imm[7:4]
// L-Type:   [3:0]=rd      [7:4]=opcode  [11:8]=imm[3:0] [12]=unused [15:13]=wrs [23:16]=imm[11:4]
// S-Type:   [3:0]=rs      [7:4]=opcode  [11:8]=imm[3:0] [12]=unused [15:13]=wrs [23:16]=imm[11:4]
// B-Type:   [3:0]=rs2     [7:4]=opcode  [11:8]=imm[3:0] [15:12]=rs1     [19:16]=funct   [23:20]=imm[7:4]
// JA-Type:  [3:1]=wrd     [7:4]=opcode  [15:8]=imm[7:0] [23:16]=imm[15:8]
// JR-Type:  [3:1]=wrd     [7:4]=opcode  [15:8]=imm[7:0] [23:16]=imm[15:8]
// JRP-Type: [3:1]=wrd     [7:4]=opcode  [15:8]=imm[7:0] [16]=unused [19:17]=wrs [23:20]=imm[11:8]
// SYSTEM:   [7:4]=opcode  [19:16]=funct

namespace thpg::wa824 {
	class ImmGen : public sc_module {
	public:
		sc_in<sc_lv<24>> inst{"INST"};

		// I-Type: imm[7:0] signed, {imm[7:4]=inst[23:20], imm[3:0]=inst[11:8]}
		sc_out<sc_lv<8>>  imme_i  {"IMME_I"};

		// L/S-Type: imm[11:0] signed, {imm[11:4]=inst[23:16], imm[3:0]=inst[11:8]}
		sc_out<sc_lv<12>> imme_ls {"IMME_LS"};

		// B-Type: imm[7:0] signed, {imm[7:4]=inst[23:20], imm[3:0]=inst[11:8]}
		sc_out<sc_lv<8>>  imme_b  {"IMME_B"};

		// JA/JR-Type: imm[15:0], {imm[15:8]=inst[23:16], imm[7:0]=inst[15:8]}
		sc_out<sc_lv<16>> imme_ja {"IMME_JA"};

		// JRP-Type: imm[11:0] signed, {imm[11:8]=inst[23:20], imm[7:0]=inst[15:8]}
		sc_out<sc_lv<12>> imme_jrp{"IMME_JRP"};

		ImmGen(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
