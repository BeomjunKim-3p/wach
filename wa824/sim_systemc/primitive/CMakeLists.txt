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
			"public/mux_nto1.cppm"
			"public/demux_1ton.cppm"
			"public/d_flip_flop.cppm"
	PRIVATE
		"private/three_state_buffer.cpp"
		"private/d_flip_flop.cpp"
)

target_link_libraries(primitive PUBLIC WA824)


