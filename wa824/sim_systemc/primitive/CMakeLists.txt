add_library(primitive)

set_target_properties(primitive PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_SCAN_FOR_MODULES ON
)

add_subdirectory(test)

target_sources(primitive
	PUBLIC
		FILE_SET CXX_MODULES
		BASE_DIRS "public"
		FILES
			"public/three_state_buffer.cppm"
	PRIVATE
		"private/three_state_buffer.cpp"
)

target_link_libraries(primitive PUBLIC WA824)


