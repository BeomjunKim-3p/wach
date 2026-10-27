#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_mem(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "DCache", "addr",      lv_hex(top.dcache.addr.read(), 4));
	add(snapshot, "DCache", "wdata",     lv_hex(top.dcache.wdata.read(), 2));
	add(snapshot, "DCache", "mem_read",  bool_str(top.dcache.mem_read.read()));
	add(snapshot, "DCache", "mem_write", bool_str(top.dcache.mem_write.read()));
	add(snapshot, "DCache", "rdata",     lv_hex(top.dcache.rdata.read(), 2));
	add(snapshot, "DCache", "hit",       bool_str(top.dcache.hit.read()));
	add(snapshot, "DCache", "dc_addr",   lv_hex(top.dcache.dc_addr.read(), 4));
	add(snapshot, "DCache", "dc_wdata",  lv_hex(top.dcache.dc_wdata.read(), 2));
	add(snapshot, "DCache", "dc_we",     bool_str(top.dcache.dc_we.read()));
	add(snapshot, "DCache", "dc_re",     bool_str(top.dcache.dc_re.read()));
	add(snapshot, "DCache", "dc_rdata",  lv_hex(top.dcache.dc_rdata.read(), 2));
	add(snapshot, "DCache", "dc_ready",  bool_str(top.dcache.dc_ready.read()));
	add(snapshot, "DCache", "dc_grant",  bool_str(top.dcache.dc_grant.read()));
	add(snapshot, "DCache", "int_req",   bool_str(top.dc_mem_read.read()));
	add(snapshot, "DCache", "int_we",    bool_str(top.dc_mem_write.read()));

	add(snapshot, "MemArbiter", "mem_addr",  lv_hex(top.mem_arbiter.mem_addr.read(), 4));
	add(snapshot, "MemArbiter", "mem_wdata", lv_hex(top.mem_arbiter.mem_wdata.read(), 2));
	add(snapshot, "MemArbiter", "mem_we",    bool_str(top.mem_arbiter.mem_we.read()));
	add(snapshot, "MemArbiter", "mem_re",    bool_str(top.mem_arbiter.mem_re.read()));
	add(snapshot, "MemArbiter", "mem_rdata", lv_hex(top.mem_arbiter.mem_rdata.read(), 2));
	add(snapshot, "MemArbiter", "mem_ready", bool_str(top.mem_arbiter.mem_ready.read()));
	add(snapshot, "MemArbiter", "ic_addr",   lv_hex(top.mem_arbiter.ic_addr.read(), 4));
	add(snapshot, "MemArbiter", "ic_re",     bool_str(top.mem_arbiter.ic_re.read()));
	add(snapshot, "MemArbiter", "ic_rdata",  lv_hex(top.mem_arbiter.ic_rdata.read(), 2));
	add(snapshot, "MemArbiter", "ic_ready",  bool_str(top.mem_arbiter.ic_ready.read()));
	add(snapshot, "MemArbiter", "ic_grant",  bool_str(top.mem_arbiter.ic_grant.read()));
	add(snapshot, "MemArbiter", "dc_addr",   lv_hex(top.mem_arbiter.dc_addr.read(), 4));
	add(snapshot, "MemArbiter", "dc_wdata",  lv_hex(top.mem_arbiter.dc_wdata.read(), 2));
	add(snapshot, "MemArbiter", "dc_we",     bool_str(top.mem_arbiter.dc_we.read()));
	add(snapshot, "MemArbiter", "dc_re",     bool_str(top.mem_arbiter.dc_re.read()));
	add(snapshot, "MemArbiter", "dc_rdata",  lv_hex(top.mem_arbiter.dc_rdata.read(), 2));
	add(snapshot, "MemArbiter", "dc_ready",  bool_str(top.mem_arbiter.dc_ready.read()));
	add(snapshot, "MemArbiter", "dc_grant",  bool_str(top.mem_arbiter.dc_grant.read()));

	add(snapshot, "MEM/WB", "valid",   bool_str(top.mem_wb_valid.read()));
	add(snapshot, "MEM/WB", "pc",      lv_hex(top.mem_wb_pc.read(), 4));
	add(snapshot, "MEM/WB", "pc3",     lv_hex(top.mem_wb_pc3.read(), 4));
	add(snapshot, "MEM/WB", "ctrl",    lv_hex(top.mem_wb_ctrl.read(), 4));
	add(snapshot, "MEM/WB", "rd_sel",  lv_hex(top.mem_wb_rd_sel.read(), 1));
	add(snapshot, "MEM/WB", "alu_out", lv_hex(top.mem_wb_alu_out.read(), 2));
	add(snapshot, "MEM/WB", "mem_out", lv_hex(top.mem_wb_mem_out.read(), 2));
}

} // namespace thpg::wa824
