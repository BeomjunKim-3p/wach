function(add_primitive_test TEST_NAME TEST_SOURCE)
	set(EXEC_NAME "test-${TEST_NAME}")
	add_executable(${EXEC_NAME} ${TEST_SOURCE})
	target_link_libraries(${EXEC_NAME} PRIVATE WA824 primitive)
	add_test(NAME ${TEST_NAME} COMMAND ${EXEC_NAME})
	set_target_properties(${EXEC_NAME} PROPERTIES
		CXX_STANDARD 23
		CXX_SCAN_FOR_MODULES ON
	)
endfunction()

set(testlistfile "${CMAKE_CURRENT_SOURCE_DIR}/CTestLists.txt")

file(STRINGS ${testlistfile} TESTLISTS
	REGEX "^[^#]"
)


foreach(testline IN LISTS TESTLISTS)
	STRING(STRIP "${testline}" testline)
	separate_arguments(test_argv UNIX_COMMAND "${testline}")
	list(LENGTH test_argv test_argc)
	if(NOT test_argc EQUAL 2)
		continue()
	endif()
	list(GET test_argv 0 name)
	list(GET test_argv 1 source)
	add_primitive_test(${name} ${source})
endforeach()
