# Aegis Heuristics - Micro-Benchmarking Configuration

# Gather all benchmark source files
file(GLOB BENCHMARK_SOURCES "*.cpp")

# Create the benchmark executable
add_executable(aegis_benchmarks ${BENCHMARK_SOURCES})

# Link against:
# 1. aegis_core: The monolithic, fully inlined engine.
# 2. benchmark::benchmark_main: Provides the main() function for the benchmark runner.
# 3. benchmark::benchmark: The core framework.
target_link_libraries(aegis_benchmarks 
    PRIVATE 
    aegis_core 
    benchmark::benchmark_main 
    benchmark::benchmark
)