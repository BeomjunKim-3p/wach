add_library(primitive)

set_target_properties(primitive PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_SCAN_FOR_MODULES ON
)

target_sources(primitive
	PUBLIC
		FILE_SET CXX_MODULES
		BASE_DIRS "public"
		FILES

	PRIVATE
)

target_link_libraries(primitive PUBLIC WA824)


