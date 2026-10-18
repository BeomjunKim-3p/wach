#include <systemc.h>
#include "register_file_redge.h"

thpg::wa824::RegisterFileREdge::RegisterFileREdge(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh_cs);
	sensitive << rd_addr << rd_we_l << rd_we_h << rd_data_l << rd_data_h;

	SC_METHOD(refresh_read);
	sensitive << rs1_addr << rs2_addr << wrs_addr;
	for (int i = 0; i < 16; i++) {
		sensitive << ralwaysout[i];
	}

	rregister.init(16, [&](const char* n, size_t i) {
		return new thpg::primitive::RegisterREdge<8>(n);
	});

	for (int i = 0; i < 16; i++) {
		rregister[i].cs      (rcs[i]);
		rregister[i].wr_bar  (rwr_bar[i]);
		rregister[i].data    (rdata_in[i]);
		rregister[i].alwaysout(ralwaysout[i]);
		rregister[i].clk     (clk);
		rregister[i].reset   (reset);
	}

	if (tf) {
		std::string mn = std::string(this->name()) + "(RegisterFileREdge)";
		sc_trace(tf, rs1_addr,  (mn + ".RS1_ADDR").c_str());
		sc_trace(tf, rs1_data,  (mn + ".RS1_DATA").c_str());
		sc_trace(tf, rs2_addr,  (mn + ".RS2_ADDR").c_str());
		sc_trace(tf, rs2_data,  (mn + ".RS2_DATA").c_str());
		sc_trace(tf, wrs_addr,  (mn + ".WRS_ADDR").c_str());
		sc_trace(tf, wrs_data,  (mn + ".WRS_DATA").c_str());
		sc_trace(tf, rd_addr,   (mn + ".RD_ADDR").c_str());
		sc_trace(tf, rd_data_l, (mn + ".RD_DATA_L").c_str());
		sc_trace(tf, rd_data_h, (mn + ".RD_DATA_H").c_str());
		sc_trace(tf, rd_we_l,   (mn + ".RD_WE_L").c_str());
		sc_trace(tf, rd_we_h,   (mn + ".RD_WE_H").c_str());
		sc_trace(tf, clk,       (mn + ".CLK").c_str());
		sc_trace(tf, reset,     (mn + ".RESET").c_str());
		for (int i = 0; i < 16; i++) {
			sc_trace(tf, rcs[i],       (mn + ".rcs["       + std::to_string(i) + "]").c_str());
			sc_trace(tf, rwr_bar[i],   (mn + ".rwr_bar["   + std::to_string(i) + "]").c_str());
			sc_trace(tf, ralwaysout[i],(mn + ".ralwaysout["+ std::to_string(i) + "]").c_str());
		}
	}
}

void thpg::wa824::RegisterFileREdge::refresh_cs(void)
{
	// 전체 초기화: cs=0, wr_bar=0 (read 모드, cs 비활성이므로 무관)
	for (int i = 0; i < 16; i++) {
		rcs[i].write(SC_LOGIC_0);
		rwr_bar[i].write(SC_LOGIC_0);
		rdata_in[i].write(sc_lv<8>(SC_LOGIC_Z));
	}

	// low byte write
	if (rd_we_l.read()) {
		unsigned int l_idx = rd_addr.read().to_uint();
		rcs[l_idx].write(SC_LOGIC_1);
		rwr_bar[l_idx].write(SC_LOGIC_1);
		rdata_in[l_idx].write(rd_data_l.read());
	}

	// high byte write: LSB를 1로 강제 → 페어의 h 레지스터
	if (rd_we_h.read()) {
		unsigned int h_idx = (rd_addr.read().to_uint() & 0xEu) | 1u;
		rcs[h_idx].write(SC_LOGIC_1);
		rwr_bar[h_idx].write(SC_LOGIC_1);
		rdata_in[h_idx].write(rd_data_h.read());
	}
}

void thpg::wa824::RegisterFileREdge::refresh_read(void)
{
	if (rs1_addr.read().is_01()) {
		rs1_data.write(ralwaysout[rs1_addr.read().to_uint()].read());
	} else {
		rs1_data.write(sc_lv<8>(SC_LOGIC_X));
	}

	if (rs2_addr.read().is_01()) {
		rs2_data.write(ralwaysout[rs2_addr.read().to_uint()].read());
	} else {
		rs2_data.write(sc_lv<8>(SC_LOGIC_X));
	}

	if (wrs_addr.read().is_01()) {
		unsigned int pair = wrs_addr.read().to_uint();
		sc_lv<16> wide;
		// 리틀엔디안: low byte → [7:0], high byte → [15:8]
		wide.range(7,  0) = ralwaysout[pair * 2].read();
		wide.range(15, 8) = ralwaysout[pair * 2 + 1].read();
		wrs_data.write(wide);
	} else {
		wrs_data.write(sc_lv<16>(SC_LOGIC_X));
	}
}
