#pragma once

#include <systemc.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "topboard.h"

namespace thpg::wa824::debugfmt {

inline std::string bool_str(bool value)
{
	return value ? "1" : "0";
}

inline std::string logic_str(const sc_logic &value)
{
	if (value == SC_LOGIC_0) return "0";
	if (value == SC_LOGIC_1) return "1";
	if (value == SC_LOGIC_Z) return "Z";
	return "X";
}

inline std::string uhex(uint64_t value, int digits = 0)
{
	std::ostringstream os;
	os << "0x" << std::uppercase << std::hex << std::setfill('0');
	if (digits > 0) os << std::setw(digits);
	os << value;
	return os.str();
}

inline std::string udec(uint64_t value)
{
	return std::to_string(value);
}

template <int N>
std::string lv_hex(const sc_lv<N> &value, int digits = 0)
{
	if (!value.is_01()) {
		return "X";
	}
	return uhex(value.to_uint64(), digits);
}

inline void add(DebugSnapshot &snapshot,
                const std::string &group,
                const std::string &name,
                const std::string &value)
{
	for (auto &g : snapshot.groups) {
		if (g.name == group) {
			g.values.push_back(DebugValue{name, value});
			return;
		}
	}
	snapshot.groups.push_back(DebugGroup{group, {DebugValue{name, value}}});
}

} // namespace thpg::wa824::debugfmt
