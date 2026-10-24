#pragma once
#include <systemc.h>

// ISA 비트 필드 (LSB=bit0, 24bit inst):
// R-Type:   [3:0]=opcode [7:4]=rd    [11:8]=rs1   [15:12]=rs2      [19:16]=X       [23:20]=funct
// I-Type:   [3:0]=opcode [7:4]=rd    [11:8]=rs1   [15:12]=imm[3:0] [19:16]=imm[7:4][23:20]=funct
// L-Type:   [3:0]=opcode [7:4]=rd    [10:8]=wrs   [11]=X  [15:12]=imm[3:0] [23:16]=imm[11:4]
// S-Type:   [3:0]=opcode [7:4]=rs    [10:8]=wrs   [11]=X  [15:12]=imm[3:0] [23:16]=imm[11:4]
// B-Type:   [3:0]=opcode [7:4]=rs2   [11:8]=rs1   [15:12]=imm[3:0] [19:16]=imm[7:4][23:20]=funct
// JA-Type:  [3:0]=opcode [6:4]=wrd   [7]=X        [15:8]=imm[7:0]  [23:16]=imm[15:8]
// JR-Type:  [3:0]=opcode [6:4]=wrd   [7]=X        [15:8]=imm[7:0]  [23:16]=imm[15:8]
// JRP-Type: [3:0]=opcode [6:4]=wrd   [7]=X        [15:8]=imm[7:0]  [19:16]=imm[11:8][22:20]=wrs [23]=X

namespace thpg::wa824 {
	class ImmGen : public sc_module {
	public:
		sc_in<sc_lv<24>> inst{"INST"};

		// I-Type: imm[7:0] signed, {imm[7:4]=inst[19:16], imm[3:0]=inst[15:12]}
		sc_out<sc_lv<8>>  imme_i  {"IMME_I"};

		// L/S-Type: imm[11:0] signed, {imm[11:4]=inst[23:16], imm[3:0]=inst[15:12]}
		sc_out<sc_lv<12>> imme_ls {"IMME_LS"};

		// B-Type: imm[7:0] signed, {imm[7:4]=inst[19:16], imm[3:0]=inst[15:12]}
		sc_out<sc_lv<8>>  imme_b  {"IMME_B"};

		// JA/JR-Type: imm[15:0], {imm[15:8]=inst[23:16], imm[7:0]=inst[15:8]}
		sc_out<sc_lv<16>> imme_ja {"IMME_JA"};

		// JRP-Type: imm[11:0] signed, {imm[11:8]=inst[19:16], imm[7:0]=inst[15:8]}
		sc_out<sc_lv<12>> imme_jrp{"IMME_JRP"};

		ImmGen(sc_module_name name, sc_trace_file *tf = nullptr);

		void refresh(void);
	};
}
