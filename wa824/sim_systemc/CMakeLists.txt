cmake_minimum_required(VERSION 3.10)
project(wa824I CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(wa824I main.cpp)

target_include_directories(wa824I PRIVATE $ENV{SYSTEMC_HOME}/include)

target_link_directories(wa824I PRIVATE $ENV{SYSTEMC_HOME}/lib)

target_link_libraries(wa824I PRIVATE systemc m)

set_target_properties(wa824I PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ENV{SYSTEMC_HOME}/lib"
)
