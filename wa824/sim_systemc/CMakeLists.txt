cmake_minimum_required(VERSION 3.21)
project(WA824 CXX)
add_library(WA824 INTERFACE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

set(DEFAULT_CORE_MODULE "I")

set(CORE_MODULE ${DEFAULT_CORE_MODULE} CACHE STRING "Select Architecture Core Module")
set(ALLOWED_CORE_MODULES "I")
set_property(CACHE CORE_MODULE PROPERTY STRINGS ${ALLOWED_CORE_MODULES})

set(COMPONENT_MODULES "" CACHE STRING "Select Architecture Component Modules")
set(ALLOWED_COMPONENT_LIST "")
set(INVALID_COMPONENTS "")

set(TESTBENCH_MODULE "" CACHE STRING "Select Testbench Module")
set(ALLOWED_TESTBENCH_MODULES "")
set_property(CACHE CORE_MODULE PROPERTY STRINGS ${ALLOWED_TESTBENCH_MODULE})

string(STRIP "${CORE_MODULE}" CORE_MODULE)
if("${CORE_MODULE}" STREQUAL "")
    message(FATAL_ERROR 
	    "\n[CMake ERROR] CORE_MODULE is empty"
    )
endif()
if(NOT CORE_MODULE IN_LIST ALLOWED_CORE_MODULES)
    message(WARNING "Invalid input value \"${CORE_MODULE}\" was detected, reverted to the default module (${DEFAULT_CORE_MODULE}).")
    set(CORE_MODULE "${DEFAULT_CORE_MODULE}" CACHE STRING "Select Architecture Core Module" FORCE)
endif()

list(REMOVE_DUPLICATES COMPONENT_MODULES)
foreach(COMPONENT_MODULE ${COMPONENT_MODULES})
	if(NOT COMPONENT_MODULE IN_LIST ALLOWED_COMPONENT_LIST)
		list(APPEND INVALID_COMPONENTS "${COMPONENT_MODULE}")
	endif()
endforeach()
list(LENGTH INVALID_COMPONENTS ERROR_COUNT)
if(ERROR_COUNT GREATER 0)
	message(FATAL_ERROR 
		"\n[CMAKE ERROR] Find Incorrect Component Modules: '${ITEM}'"
        	"\nSelect only from the list of allowed values (separated by semicolons)."
		"\nAllowed Values: ${ALLOWED_LIST}\n"
        )
endif()

if(TESTBENCH_MODULE AND NOT TESTBENCH_MODULE IN_LIST ALLOWED_TESTBENCH_MODULES)
	message(WARNING "Invalid testbench module \"${TESTBENCH_MODULE}\" was detected, testbench module is not applied.")
	set(TESTBENCH_MODULE "" CACHE STRING "Select Testbench Module" FORCE)
endif()

# 로직게이트, 플립플롭등 최소 단위 부품 모듈
add_subdirectory(primitive)
# 코어모듈
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/core/${CORE_MODULE}")

# 컴포넌트 모듈
if(COMPONENT_MODULES)
	foreach(COMPONENT_MODULE ${COMPONENT_MODULES})
		add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/components/${COMPONENT_MODULE}")
	endforeach()
endif()
## core모듈에서 컴포넌트 모듈 감지 및 사용 위해 매크로 전달
foreach(COMP ${COMPONENT_MODULES})
	target_compile_definitions(WA824 INTERFACE
        "USE_${COMP}"
    )
endforeach()

# 테스트벤치 모듈
if(TESTBENCH_MODULE)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/testbench/${TESTBENCH_MODULE}")
	target_compile_definitions(WA824 INTERFACE "USE_TESTBENCH")
endif()

target_include_directories(WA824 INTERFACE $ENV{SYSTEMC_HOME}/include)

target_link_directories(WA824 INTERFACE $ENV{SYSTEMC_HOME}/lib)

target_link_libraries(WA824 INTERFACE systemc m)


