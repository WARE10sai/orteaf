cmake_minimum_required(VERSION 3.20)

project(orteaf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Runtime enable flags
option(ENABLE_CPU "Enable CPU runtime" ON)
option(ENABLE_CUDA "Enable CUDA runtime" OFF)
option(ENABLE_MPS "Enable Metal (MPS) runtime" OFF)

# Statistics level configuration (shared by library and tests)
set(ORTEAF_RUNTIME_STATS_LEVEL 0 CACHE STRING
    "Default statistics level for runtimes and allocators (0=off, 1=basic, 2=extended)")
set_property(CACHE ORTEAF_RUNTIME_STATS_LEVEL PROPERTY STRINGS 0 1 2)

foreach(_component CPU CUDA MPS ALLOCATOR)
    set(ORTEAF_${_component}_STATS_LEVEL "AUTO" CACHE STRING
        "Statistics level override for ${_component} (AUTO inherits ORTEAF_RUNTIME_STATS_LEVEL)")
    set_property(CACHE ORTEAF_${_component}_STATS_LEVEL PROPERTY STRINGS AUTO 0 1 2)
endforeach()

function(orteaf_resolve_stats_level out_var override_value)
    if("${override_value}" STREQUAL "AUTO")
        set(${out_var} ${ORTEAF_RUNTIME_STATS_LEVEL} PARENT_SCOPE)
    else()
        set(${out_var} ${override_value} PARENT_SCOPE)
    endif()
endfunction()

orteaf_resolve_stats_level(ORTEAF_CPU_STATS_LEVEL_EFFECTIVE "${ORTEAF_CPU_STATS_LEVEL}")
orteaf_resolve_stats_level(ORTEAF_CUDA_STATS_LEVEL_EFFECTIVE "${ORTEAF_CUDA_STATS_LEVEL}")
orteaf_resolve_stats_level(ORTEAF_MPS_STATS_LEVEL_EFFECTIVE "${ORTEAF_MPS_STATS_LEVEL}")
orteaf_resolve_stats_level(ORTEAF_ALLOCATOR_STATS_LEVEL_EFFECTIVE "${ORTEAF_ALLOCATOR_STATS_LEVEL}")

function(orteaf_bool_to_int out_var value)
    if(${value})
        set(${out_var} 1 PARENT_SCOPE)
    else()
        set(${out_var} 0 PARENT_SCOPE)
    endif()
endfunction()

orteaf_bool_to_int(ORTEAF_ENABLE_CPU_INT ENABLE_CPU)
orteaf_bool_to_int(ORTEAF_ENABLE_CUDA_INT ENABLE_CUDA)
orteaf_bool_to_int(ORTEAF_ENABLE_MPS_INT ENABLE_MPS)

add_subdirectory(orteaf)
add_subdirectory(tests)
