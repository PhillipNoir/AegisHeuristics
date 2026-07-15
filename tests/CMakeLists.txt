# Aegis Heuristics - Unit Tests Configuration


# Enable CTest for the current directory
enable_testing()

# Gather all test source files dynamically
file(GLOB TEST_SOURCES "*.cpp")

# Create the test executable
add_executable(aegis_tests ${TEST_SOURCES})

# Link the test executable against:
# 1. aegis_core: Our statically compiled monolithic engine. By linking this, 
#    we test the exact same optimized binary that runs in production.
# 2. gtest_main & gtest: The testing frameworks automatically fetched by 
#    the master CMakeLists.txt. gtest_main provides the main() function.
target_link_libraries(aegis_tests 
    PRIVATE 
    aegis_core 
    gtest_main 
    gtest
)

# Automatically scan the executable for TEST() macros and register them with CTest.
# This allows running all tests smoothly via the 'ctest' command line utility.
include(GoogleTest)
gtest_discover_tests(aegis_tests)

