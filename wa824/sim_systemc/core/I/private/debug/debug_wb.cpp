#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_wb(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "WB", "valid",  bool_str(top.mem_wb_valid.read()));
	add(snapshot, "WB", "ctrl",   lv_hex(top.mem_wb_ctrl.read(), 4));
	add(snapshot, "WB", "rd_sel", lv_hex(top.mem_wb_rd_sel.read(), 1));
	add(snapshot, "WB", "data_l", lv_hex(top.wb_data_l.read(), 2));
	add(snapshot, "WB", "data_h", lv_hex(top.wb_data_h.read(), 2));
	add(snapshot, "WB", "we4l",   bool_str(top.wb_we4l.read()));
	add(snapshot, "WB", "we4h",   bool_str(top.wb_we4h.read()));
}

} // namespace thpg::wa824
