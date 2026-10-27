#include "debug_collector.h"
#include "debug_format.h"

namespace thpg::wa824 {

void DebugCollector::collect_pc(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;
	add(snapshot, "PC", "pc",       lv_hex(top.pc_reg.read(), 4));
	add(snapshot, "PC", "next_pc",  lv_hex(top.next_pc.read(), 4));
	add(snapshot, "PC", "reset",    bool_str(top.reset.read()));
}

void DebugCollector::collect_if(Topboard &top, DebugSnapshot &snapshot)
{
	using namespace debugfmt;

	add(snapshot, "ICache", "pc",       lv_hex(top.icache.pc.read(), 4));
	add(snapshot, "ICache", "inst",     lv_hex(top.icache.inst.read(), 6));
	add(snapshot, "ICache", "hit",      bool_str(top.icache.hit.read()));
	add(snapshot, "ICache", "ic_addr",  lv_hex(top.icache.ic_addr.read(), 4));
	add(snapshot, "ICache", "ic_re",    bool_str(top.icache.ic_re.read()));
	add(snapshot, "ICache", "ic_rdata", lv_hex(top.icache.ic_rdata.read(), 2));
	add(snapshot, "ICache", "ic_ready", bool_str(top.icache.ic_ready.read()));
	add(snapshot, "ICache", "ic_grant", bool_str(top.icache.ic_grant.read()));

	add(snapshot, "BPE", "inst",               lv_hex(top.bpe.inst.read(), 6));
	add(snapshot, "BPE", "pc",                 lv_hex(top.bpe.pc.read(), 4));
	add(snapshot, "BPE", "branch_pred_result", bool_str(top.bpe.branch_pred_result.read()));

	add(snapshot, "FlowController", "inst",        lv_hex(top.fc.inst.read(), 6));
	add(snapshot, "FlowController", "branch_pred", bool_str(top.fc.branch_pred.read()));
	add(snapshot, "FlowController", "is_jalr",     bool_str(top.fc.is_jalr.read()));
	add(snapshot, "FlowController", "b_offset",    lv_hex(top.fc.b_offset.read(), 2));
	add(snapshot, "FlowController", "jr_imme",     lv_hex(top.fc.jr_imme.read(), 4));
	add(snapshot, "FlowController", "ja_imme",     lv_hex(top.fc.ja_imme.read(), 4));
	add(snapshot, "FlowController", "jump_type",   lv_hex(top.fc.jump_type.read(), 1));

	add(snapshot, "IF/ID", "valid", bool_str(top.if_id_valid.read()));
	add(snapshot, "IF/ID", "pc",    lv_hex(top.if_id_pc.read(), 4));
	add(snapshot, "IF/ID", "pc3",   lv_hex(top.if_id_pc3.read(), 4));
	add(snapshot, "IF/ID", "inst",  lv_hex(top.if_id_inst.read(), 6));
}

} // namespace thpg::wa824
