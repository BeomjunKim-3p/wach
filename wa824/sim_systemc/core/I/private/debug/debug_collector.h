#pragma once

#include "topboard.h"

namespace thpg::wa824 {

// Topboard의 private 신호를 단계/모듈별 파일에서 읽어 스냅샷으로 만든다.
struct DebugCollector {
	static void collect_pc(Topboard &top, DebugSnapshot &snapshot);
	static void collect_if(Topboard &top, DebugSnapshot &snapshot);
	static void collect_id(Topboard &top, DebugSnapshot &snapshot);
	static void collect_ex(Topboard &top, DebugSnapshot &snapshot);
	static void collect_mem(Topboard &top, DebugSnapshot &snapshot);
	static void collect_wb(Topboard &top, DebugSnapshot &snapshot);
	static void collect_hde(Topboard &top, DebugSnapshot &snapshot);
};

} // namespace thpg::wa824
