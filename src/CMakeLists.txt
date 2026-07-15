# Aegis Heuristics - Core Engine Build Configuration

# Gather all source files dynamically. 
# For a pure monolithic build, grouping everything in src/ is highly efficient.
file(GLOB AEGIS_SOURCES "*.cpp")
list(REMOVE_ITEM AEGIS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")


# Build the core algorithmic engine as a static library.
# This prevents the need to recompile the heavy heuristic logic 
# when simply updating a unit test or benchmark.
add_library(aegis_core STATIC ${AEGIS_SOURCES})

# Force the compiler to treat all sources as a single translation unit.
# This bypasses Link-Time Optimization (LTO) limits and guarantees 
# aggressive cross-file inlining for the Simulated Annealing loops.
set_target_properties(aegis_core PROPERTIES UNITY_BUILD ON)

# Publicly expose the include directory so that tests and benchmarks 
# automatically inherit the path when linking against aegis_core.
target_include_directories(aegis_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Link OpenMP specifically to the core engine to enable lock-free 
# embarrassingly parallel execution.
target_link_libraries(aegis_core PUBLIC OpenMP::OpenMP_CXX)

add_executable(aegis_run main.cpp)
target_link_libraries(aegis_run PRIVATE aegis_core)