#include "debug_collector.h"

#include "topboard.h"

namespace thpg::wa824 {

DebugSnapshot Topboard::capture_debug(void)
{
	DebugSnapshot snapshot;
	snapshot.cycle = m_cycle;

	DebugCollector::collect_pc(*this, snapshot);
	DebugCollector::collect_if(*this, snapshot);
	DebugCollector::collect_id(*this, snapshot);
	DebugCollector::collect_ex(*this, snapshot);
	DebugCollector::collect_mem(*this, snapshot);
	DebugCollector::collect_wb(*this, snapshot);
	DebugCollector::collect_hde(*this, snapshot);

	return snapshot;
}

} // namespace thpg::wa824
