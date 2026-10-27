#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_ex(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "ALU", "a",        lv_hex(top.alu.a.read(), 2));
	add(snapshot, "ALU", "b",        lv_hex(top.alu.b.read(), 2));
	add(snapshot, "ALU", "funct",    lv_hex(top.alu.funct.read(), 1));
	add(snapshot, "ALU", "carry_in", logic_str(top.alu.carry_in.read()));
	add(snapshot, "ALU", "y",        lv_hex(top.alu.y.read(), 2));
	add(snapshot, "ALU", "flags",    lv_hex(top.alu.flags.read(), 1));

	add(snapshot, "AddrAdder", "wrs_data", lv_hex(top.id_ex_wrs_data.read(), 4));
	add(snapshot, "AddrAdder", "imme",     lv_hex(top.id_ex_imme.read(), 3));
	add(snapshot, "AddrAdder", "addr_out", lv_hex(top.ex_addr_out.read(), 4));

	add(snapshot, "EX/Forwarding", "ex_rs1_data",       lv_hex(top.ex_rs1_data.read(), 2));
	add(snapshot, "EX/Forwarding", "ex_rs2_data",       lv_hex(top.ex_rs2_data.read(), 2));
	add(snapshot, "EX/Forwarding", "ex_alu_b",          lv_hex(top.ex_alu_b.read(), 2));
	add(snapshot, "EX/Forwarding", "ex_rs1_wb_fwd_sel", lv_hex(top.hde_ex_rs1_fwd_sel.read(), 1));
	add(snapshot, "EX/Forwarding", "ex_rs2_wb_fwd_sel", lv_hex(top.hde_ex_rs2_fwd_sel.read(), 1));
	add(snapshot, "EX/Forwarding", "ex_wrd_fwd_sel",    lv_hex(top.hde_ex_wrd_fwd_sel.read(), 1));
	add(snapshot, "EX/Forwarding", "alu_out",           lv_hex(top.alu_out_sig.read(), 2));
	add(snapshot, "EX/Forwarding", "addr_out",          lv_hex(top.ex_addr_out.read(), 4));

	add(snapshot, "EX/MEM", "valid",    bool_str(top.ex_mem_valid.read()));
	add(snapshot, "EX/MEM", "pc",       lv_hex(top.ex_mem_pc.read(), 4));
	add(snapshot, "EX/MEM", "pc3",      lv_hex(top.ex_mem_pc3.read(), 4));
	add(snapshot, "EX/MEM", "ctrl",     lv_hex(top.ex_mem_ctrl.read(), 4));
	add(snapshot, "EX/MEM", "rd_sel",   lv_hex(top.ex_mem_rd_sel.read(), 1));
	add(snapshot, "EX/MEM", "alu_out",  lv_hex(top.ex_mem_alu_out.read(), 2));
	add(snapshot, "EX/MEM", "addr",     lv_hex(top.ex_mem_addr.read(), 4));
	add(snapshot, "EX/MEM", "rs_store", lv_hex(top.ex_mem_rs_store.read(), 2));
}

} // namespace thpg::wa824
