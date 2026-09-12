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
)

target_link_libraries(WA824I PRIVATE primitive WA824)

install(TARGETS WA824I
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
)

