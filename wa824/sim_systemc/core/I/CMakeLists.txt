add_library(WA824I)

set_target_properties(WA824I PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_SCAN_FOR_MODULES ON
)

target_include_directories(WA824I PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>"
	"$<INSTALL_INTERFACE:include>"
)

target_sources(WA824I
	PUBLIC
		FILE_SET HEADERS BASE_DIRS "public"
		FILES
			"public/topboard.h"
			"public/alu.h"
			"public/branch_comparator.h"
			"public/branch_pred_engine.h"
			"public/control_unit.h"
			"public/dcache.h"
			"public/flow_controller.h"
			"public/hazard_engine.h"
			"public/icache.h"
			"public/imm_gen.h"
			"public/mem_arbiter.h"
			"public/register_file_redge.h"
	PRIVATE
		"private/alu.cpp"
		"private/branch_comparator.cpp"
		"private/branch_pred_engine.cpp"
		"private/control_unit.cpp"
		"private/dcache.cpp"
		"private/debug/debug_collector.h"
		"private/debug/debug_format.h"
		"private/debug/debug_snapshot.cpp"
		"private/debug/debug_if.cpp"
		"private/debug/debug_id.cpp"
		"private/debug/debug_ex.cpp"
		"private/debug/debug_mem.cpp"
		"private/debug/debug_wb.cpp"
		"private/debug/debug_hde.cpp"
		"private/flow_controller.cpp"
		"private/hazard_engine.cpp"
		"private/icache.cpp"
		"private/imm_gen.cpp"
		"private/mem_arbiter.cpp"
		"private/register_file_redge.cpp"
		"private/topboard.cpp"
)

target_link_libraries(WA824I PUBLIC primitive WA824)

add_subdirectory(test)

install(TARGETS WA824I
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	FILE_SET HEADERS DESTINATION include
)
