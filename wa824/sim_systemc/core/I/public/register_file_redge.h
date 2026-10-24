#pragma once
#include <systemc.h>

import primitive.register_redge;

namespace thpg::wa824 {
	class RegisterFileREdge : public sc_module {
	public:
		// 8bit read port 1 (RS1)
		sc_in<sc_lv<4>>   rs1_addr{"RS1_ADDR"};
		sc_out<sc_lv<8>>  rs1_data{"RS1_DATA"};

		// 8bit read port 2 (RS2)
		sc_in<sc_lv<4>>   rs2_addr{"RS2_ADDR"};
		sc_out<sc_lv<8>>  rs2_data{"RS2_DATA"};

		// 16bit read port (WRS)
		sc_in<sc_lv<3>>   wrs_addr{"WRS_ADDR"};
		sc_out<sc_lv<16>> wrs_data{"WRS_DATA"};

		// write port (RD) - 8bit or 16bit pair
		// 16bit write: rd_we_l=1 && rd_we_h=1, rd_addr는 XXX0으로 들어옴 (ISA 보장)
		// 8bit l write: rd_we_l=1, rd_addr 4비트 그대로
		// 8bit h write: rd_we_h=1, rd_addr LSB를 1로 강제 처리
		sc_in<sc_lv<4>>  rd_addr  {"RD_ADDR"};
		sc_in<sc_lv<8>>  rd_data_l{"RD_DATA_L"};
		sc_in<sc_lv<8>>  rd_data_h{"RD_DATA_H"};
		sc_in<bool>      rd_we_l  {"RD_WE_L"};   // WR4L
		sc_in<bool>      rd_we_h  {"RD_WE_H"};   // WR4H

		sc_in<bool> clk  {"CLK"};
		sc_in<bool> reset{"RESET"};

		RegisterFileREdge(sc_module_name name, sc_trace_file *tf = nullptr);

		// combinational: cs/wr_bar/rdata_in 제어
		void refresh_cs(void);
		// combinational: alwaysout → rs1/rs2/wrs 멀티플렉싱
		void refresh_read(void);

		// 디버거용 레지스터 내용 조회 (0..15, x0l..x7h)
		sc_lv<8> debug_reg(unsigned int index) const {
			return index < 16 ? ralwaysout[index].read() : sc_lv<8>(SC_LOGIC_X);
		}

	private:
		sc_vector<thpg::primitive::RegisterREdge<8>> rregister{"RF_REGISTER"};
		sc_vector<sc_signal<sc_logic>>  rcs       {"RF_CS",        16};
		sc_vector<sc_signal<sc_logic>>  rwr_bar   {"RF_WR",        16};
		sc_vector<sc_signal_rv<8>>      rdata_in  {"RF_DATA_IN",   16};
		sc_vector<sc_signal<sc_lv<8>>>  ralwaysout{"RF_ALWAYSOUT", 16};
	};
}
