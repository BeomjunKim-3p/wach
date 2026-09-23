cmake_minimum_required(VERSION 3.20)

project(wa824i-as
	VERSION 0.1.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


# =========================
# Assembler Core
# =========================


add_library(as_core STATIC
	src/pp/pp.c
	src/lexer/lexer.c
	src/as/err.c
)

target_include_directories(as_core
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(as_core
	PRIVATE
		-Wall
		-Wextra
		-Wpedantic
)

# =========================
# Executable
# =========================

add_executable(wa824i-as
    src/main.c
)

target_link_libraries(wa824i-as
    PRIVATE
        as_core
)

target_compile_options(wa824i-as
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)
