add_library(WA824I)

set_target_properties(WA824I PROPERTIES
	CXX_STANDARD 23
	CXX_SCAN_FOR_MODULES ON
)

add_subdirectory(test)

target_sources(WA824I
	PUBLIC
		FILE_SET HEADERS BASE_DIRS "public"
		FILES
	PRIVATE
		"private/alu.h"
		"private/alu.cpp"
		"private/branch_comparator.h"
		"private/branch_comparator.cpp"
		"private/branch_pred_engine.h"
		"private/branch_pred_engine.cpp"
		"private/control_unit.h"
		"private/control_unit.cpp"
		"private/dcache.h"
		"private/dcache.cpp"
		"private/flow_controller.h"
		"private/flow_controller.cpp"
		"private/hazard_engine.h"
		"private/hazard_engine.cpp"
		"private/icache.h"
		"private/icache.cpp"
		"private/imm_gen.h"
		"private/imm_gen.cpp"
		"private/mem_arbiter.h"
		"private/mem_arbiter.cpp"
		"private/topboard.h"
		"private/topboard.cpp"
)

target_link_libraries(WA824I PRIVATE primitive WA824)

install(TARGETS WA824I
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)
