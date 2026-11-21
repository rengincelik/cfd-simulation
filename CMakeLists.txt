cmake_minimum_required(VERSION 3.16)
project(cfd CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Include paths ─────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# ── Browser build (Emscripten) ────────────────────────────────────────────────
#   emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm
# Writes docs/cavity.js + docs/cavity.wasm, next to docs/index.html (GitHub Pages).
# The page only needs the solver, so none of the SDL2 apps are built here.
if(EMSCRIPTEN)
    add_executable(cavity_wasm web/cavity_wasm.cpp)
    set_target_properties(cavity_wasm PROPERTIES
        OUTPUT_NAME cavity
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)
    target_compile_options(cavity_wasm PRIVATE -O3 -fexceptions -Wall -Wextra)
    target_link_options(cavity_wasm PRIVATE
        -fexceptions
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME=createCfdModule
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        -sEXPORTED_FUNCTIONS=_cfd_init,_cfd_reset,_cfd_step,_cfd_nx,_cfd_ny,_cfd_lx,_cfd_ly,_cfd_field,_cfd_render_rgba,_cfd_stats,_cfd_ghia
        -sEXPORTED_RUNTIME_METHODS=cwrap,HEAPU8,HEAPF64)
    return()
endif()

# ── SDL2 (native apps) ────────────────────────────────────────────────────────
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

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
# ─────────────────────────────────────────────────────────────────────────────
