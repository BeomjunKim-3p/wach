#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_id(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "ControlUnit", "inst", lv_hex(top.cu.inst.read(), 6));
	add(snapshot, "ControlUnit", "ctrl", lv_hex(top.cu.ctrl.read(), 4));

	add(snapshot, "ImmGen", "inst",     lv_hex(top.immgen.inst.read(), 6));
	add(snapshot, "ImmGen", "imme_i",   lv_hex(top.immgen.imme_i.read(), 2));
	add(snapshot, "ImmGen", "imme_ls",  lv_hex(top.immgen.imme_ls.read(), 3));
	add(snapshot, "ImmGen", "imme_jrp", lv_hex(top.immgen.imme_jrp.read(), 3));

	add(snapshot, "RegisterFile", "rs1_addr",  lv_hex(top.regfile.rs1_addr.read(), 1));
	add(snapshot, "RegisterFile", "rs2_addr",  lv_hex(top.regfile.rs2_addr.read(), 1));
	add(snapshot, "RegisterFile", "wrs_addr",  lv_hex(top.regfile.wrs_addr.read(), 1));
	add(snapshot, "RegisterFile", "rd_addr",   lv_hex(top.regfile.rd_addr.read(), 1));
	add(snapshot, "RegisterFile", "rs1_data",  lv_hex(top.regfile.rs1_data.read(), 2));
	add(snapshot, "RegisterFile", "rs2_data",  lv_hex(top.regfile.rs2_data.read(), 2));
	add(snapshot, "RegisterFile", "wrs_data",  lv_hex(top.regfile.wrs_data.read(), 4));
	add(snapshot, "RegisterFile", "rd_data_l", lv_hex(top.regfile.rd_data_l.read(), 2));
	add(snapshot, "RegisterFile", "rd_data_h", lv_hex(top.regfile.rd_data_h.read(), 2));
	add(snapshot, "RegisterFile", "rd_we_l",   bool_str(top.regfile.rd_we_l.read()));
	add(snapshot, "RegisterFile", "rd_we_h",   bool_str(top.regfile.rd_we_h.read()));
	add(snapshot, "RegisterFile", "addr1",     lv_hex(top.rf_addr1.read(), 1));
	add(snapshot, "RegisterFile", "addr2_mux", lv_hex(top.rf_addr2_mux.read(), 1));
	add(snapshot, "RegisterFile", "addr3_mux", lv_hex(top.rf_addr3_mux.read(), 1));
	for (unsigned int i = 0; i < 16; ++i) {
		const std::string name = "x" + std::to_string(i / 2) + (i & 1 ? "h" : "l");
		add(snapshot, "RegisterFile", name, lv_hex(top.regfile.debug_reg(i), 2));
	}

	add(snapshot, "BranchComparator", "rs1",   lv_hex(top.branch_cmp.rs1.read(), 2));
	add(snapshot, "BranchComparator", "rs2",   lv_hex(top.branch_cmp.rs2.read(), 2));
	add(snapshot, "BranchComparator", "funct", lv_hex(top.branch_cmp.funct.read(), 1));
	add(snapshot, "BranchComparator", "taken", logic_str(top.branch_cmp.taken.read()));

	add(snapshot, "ID/Forwarding", "rf_rs1_raw",       lv_hex(top.rf_rs1_raw.read(), 2));
	add(snapshot, "ID/Forwarding", "rf_rs2_raw",       lv_hex(top.rf_rs2_raw.read(), 2));
	add(snapshot, "ID/Forwarding", "rf_wrs_raw",       lv_hex(top.rf_wrs_raw.read(), 4));
	add(snapshot, "ID/Forwarding", "id_rs1_data",      lv_hex(top.id_rs1_data.read(), 2));
	add(snapshot, "ID/Forwarding", "id_rs2_data",      lv_hex(top.id_rs2_data.read(), 2));
	add(snapshot, "ID/Forwarding", "id_wrs_data",      lv_hex(top.id_wrs_data.read(), 4));
	add(snapshot, "ID/Forwarding", "id_imme_sel",      lv_hex(top.id_imme_sel.read(), 3));
	add(snapshot, "ID/Forwarding", "id_rs1_wb_fwd",    bool_str(top.hde_id_rs1_wb_fwd.read()));
	add(snapshot, "ID/Forwarding", "id_rs2_wb_fwd",    bool_str(top.hde_id_rs2_wb_fwd.read()));
	add(snapshot, "ID/Forwarding", "id_wrd_fwd_sel",   lv_hex(top.hde_id_wrd_fwd_sel.read(), 1));

	add(snapshot, "ID/EX", "valid",    bool_str(top.id_ex_valid.read()));
	add(snapshot, "ID/EX", "pc",       lv_hex(top.id_ex_pc.read(), 4));
	add(snapshot, "ID/EX", "pc3",      lv_hex(top.id_ex_pc3.read(), 4));
	add(snapshot, "ID/EX", "ctrl",     lv_hex(top.id_ex_ctrl.read(), 4));
	add(snapshot, "ID/EX", "rd_sel",   lv_hex(top.id_ex_rd_sel.read(), 1));
	add(snapshot, "ID/EX", "rs1",      lv_hex(top.id_ex_rs1.read(), 2));
	add(snapshot, "ID/EX", "rs2",      lv_hex(top.id_ex_rs2.read(), 2));
	add(snapshot, "ID/EX", "rs_store", lv_hex(top.id_ex_rs_store.read(), 2));
	add(snapshot, "ID/EX", "wrs_data", lv_hex(top.id_ex_wrs_data.read(), 4));
	add(snapshot, "ID/EX", "imme",     lv_hex(top.id_ex_imme.read(), 3));
	add(snapshot, "ID/EX", "opcode",   lv_hex(top.id_ex_opcode.read(), 1));
	add(snapshot, "ID/EX", "funct",    lv_hex(top.id_ex_funct.read(), 1));
}

} // namespace thpg::wa824
