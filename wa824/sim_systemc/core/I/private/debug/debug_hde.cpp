#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_hde(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "HDE", "icache_hit",         bool_str(top.hde.icache_hit.read()));
	add(snapshot, "HDE", "dcache_hit",         bool_str(top.hde.dcache_hit.read()));
	add(snapshot, "HDE", "branch_pred_result", bool_str(top.hde.branch_pred_result.read()));
	add(snapshot, "HDE", "branch_result",      logic_str(top.hde.branch_result.read()));
	add(snapshot, "HDE", "is_jalr",            bool_str(top.hde.is_jalr.read()));
	add(snapshot, "HDE", "inst",               lv_hex(top.hde.inst.read(), 6));
	add(snapshot, "HDE", "if_inst",            lv_hex(top.hde.if_inst.read(), 6));
	add(snapshot, "HDE", "if_pc",              lv_hex(top.hde.if_pc.read(), 4));
	add(snapshot, "HDE", "id_ex_ctrl",         lv_hex(top.hde.id_ex_ctrl.read(), 4));
	add(snapshot, "HDE", "id_ex_rd",           lv_hex(top.hde.id_ex_rd.read(), 1));
	add(snapshot, "HDE", "id_ex_wrs",          lv_hex(top.hde.id_ex_wrs.read(), 1));
	add(snapshot, "HDE", "ex_mem_ctrl",        lv_hex(top.hde.ex_mem_ctrl.read(), 4));
	add(snapshot, "HDE", "ex_mem_rd",          lv_hex(top.hde.ex_mem_rd.read(), 1));
	add(snapshot, "HDE", "ex_mem_we4l",        bool_str(top.hde.ex_mem_we4l.read()));
	add(snapshot, "HDE", "ex_mem_we4h",        bool_str(top.hde.ex_mem_we4h.read()));
	add(snapshot, "HDE", "mem_wb_rd",          lv_hex(top.hde.mem_wb_rd.read(), 1));
	add(snapshot, "HDE", "mem_wb_we4l",        bool_str(top.hde.mem_wb_we4l.read()));
	add(snapshot, "HDE", "mem_wb_we4h",        bool_str(top.hde.mem_wb_we4h.read()));

	add(snapshot, "HDE", "pc_stall",           bool_str(top.hde.pc_stall.read()));
	add(snapshot, "HDE", "if_id_stall",        bool_str(top.hde.if_id_stall.read()));
	add(snapshot, "HDE", "if_id_flush",        bool_str(top.hde.if_id_flush.read()));
	add(snapshot, "HDE", "id_ex_stall",        bool_str(top.hde.id_ex_stall.read()));
	add(snapshot, "HDE", "id_ex_flush",        bool_str(top.hde.id_ex_flush.read()));
	add(snapshot, "HDE", "ex_mem_stall",       bool_str(top.hde.ex_mem_stall.read()));
	add(snapshot, "HDE", "ex_mem_flush",       bool_str(top.hde.ex_mem_flush.read()));
	add(snapshot, "HDE", "mem_wb_stall",       bool_str(top.hde.mem_wb_stall.read()));
	add(snapshot, "HDE", "mem_wb_flush",       bool_str(top.hde.mem_wb_flush.read()));
	add(snapshot, "HDE", "not_taken_restore",  bool_str(top.hde.not_taken_restore.read()));
	add(snapshot, "HDE", "taken_restore",      bool_str(top.hde.taken_restore.read()));

	add(snapshot, "HDE", "id_rs1_wb_fwd",      bool_str(top.hde.id_rs1_wb_fwd.read()));
	add(snapshot, "HDE", "id_rs2_wb_fwd",      bool_str(top.hde.id_rs2_wb_fwd.read()));
	add(snapshot, "HDE", "ex_rs1_wb_fwd_sel",  lv_hex(top.hde.ex_rs1_wb_fwd_sel.read(), 1));
	add(snapshot, "HDE", "ex_rs2_wb_fwd_sel",  lv_hex(top.hde.ex_rs2_wb_fwd_sel.read(), 1));
	add(snapshot, "HDE", "id_wrd_fwd_sel",     lv_hex(top.hde.id_wrd_fwd_sel.read(), 1));
	add(snapshot, "HDE", "ex_wrd_fwd_sel",     lv_hex(top.hde.ex_wrd_fwd_sel.read(), 1));

	add(snapshot, "HDE", "pred_valid",        bool_str(top.hde.pred_valid()));
	add(snapshot, "HDE", "pred_taken",        bool_str(top.hde.pred_taken()));
	add(snapshot, "HDE", "pred_pc",           lv_hex(top.hde.pred_pc(), 4));
}

} // namespace thpg::wa824
