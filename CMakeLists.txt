cmake_minimum_required(VERSION 3.16)
project(cfd CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── SDL2 ─────────────────────────────────────────────────────────────────────
find_package(SDL2 REQUIRED)

# ── Include paths ─────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

# ── Helper macro ──────────────────────────────────────────────────────────────
macro(cfd_app target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE ${SDL2_LIBRARIES})
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-O2 -Wall -Wextra>
        $<$<CXX_COMPILER_ID:MSVC>:/O2 /W4>)
endmacro()

# ── Executables ───────────────────────────────────────────────────────────────
cfd_app(cavity          apps/cavity_main.cpp)
cfd_app(heat2d          apps/heat2d_main.cpp)
cfd_app(heat2d_disk     apps/heat2d_disk_main.cpp)
cfd_app(heat2d_obstacle apps/heat2d_obstacle_main.cpp)

# Copy config files to build dir
foreach(cfg cavity heat2d heat2d_disk heat2d_obstacle)
    configure_file(${CMAKE_SOURCE_DIR}/${cfg}.cfg
                   ${CMAKE_BINARY_DIR}/${cfg}.cfg COPYONLY)
endforeach()
