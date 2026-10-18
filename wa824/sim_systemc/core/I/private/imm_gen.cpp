#include <systemc.h>
#include "imm_gen.h"

thpg::wa824::ImmGen::ImmGen(sc_module_name name, sc_trace_file *tf)
	: sc_module(name)
{
	SC_METHOD(refresh);
	sensitive << inst;

	if (tf) {
		std::string mn = std::string(this->name()) + "(ImmGen)";
		sc_trace(tf, inst,     (mn + ".INST").c_str());
		sc_trace(tf, imme_i,   (mn + ".IMME_I").c_str());
		sc_trace(tf, imme_ls,  (mn + ".IMME_LS").c_str());
		sc_trace(tf, imme_ja,  (mn + ".IMME_JA").c_str());
		sc_trace(tf, imme_jrp, (mn + ".IMME_JRP").c_str());
	}
}

void thpg::wa824::ImmGen::refresh(void)
{
	sc_lv<24> i = inst.read();

	// I-Type: {inst[19:16], inst[15:12]}
	sc_lv<8> vi;
	vi.range(3, 0) = i.range(15, 12);
	vi.range(7, 4) = i.range(19, 16);
	imme_i.write(vi);

	// L/S-Type: {inst[23:16], inst[15:12]}
	sc_lv<12> vls;
	vls.range(3,  0) = i.range(15, 12);
	vls.range(11, 4) = i.range(23, 16);
	imme_ls.write(vls);


	// JA/JR-Type: {inst[23:16], inst[15:8]}
	sc_lv<16> vja;
	vja.range(7,  0) = i.range(15, 8);
	vja.range(15, 8) = i.range(23, 16);
	imme_ja.write(vja);

	// JRP-Type: {inst[19:16], inst[15:8]}
	sc_lv<12> vjrp;
	vjrp.range(7,  0) = i.range(15, 8);
	vjrp.range(11, 8) = i.range(19, 16);
	imme_jrp.write(vjrp);
}
