# AegisHeuristics

![C++](https://img.shields.io/badge/C++-23-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)
![OpenMP](https://img.shields.io/badge/OpenMP-Parallel-orange.svg)
![Google Test](https://img.shields.io/badge/Google_Test-1.12.1-orange.svg)
![Google Benchmark](https://img.shields.io/badge/Google_Benchmark-v1.8.3-red.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)

High-performance C++ optimization engine for 2D rectangular packing utilizing Simulated Annealing, bitwise compiler intrinsics, and Data-Oriented Design (DOD).

## Overview

AegisHeuristics is a high-performance optimization engine engineered to solve specific instances of the 2D rectangular packing problem, focusing on non-guillotineable layouts where items can be rotated by 90 degrees. Specifically targeting bounding boxes of 20x20 units (Category 1) and 40x15 units (Category 2), the system is designed to maximize packing density while virtually eliminating memory latency to achieve an optimal mathematical balance between raw execution speed and global optimality.

Drawing on low-level engine architecture principles, the system abandons traditional object-oriented programming in favor of pure Data-Oriented Design (DOD). The packing environment is represented through cache-aligned, one-dimensional 64-bit integer arrays (Bitboards), guaranteeing absolute spatial locality in the CPU's L1 cache. By decoupling piece attributes into a Structure of Arrays (SoA) layout and utilizing a CMake-configured Unity Build, the engine achieves $O(1)$ collision detection via hardware-accelerated bitwise intrinsics and scales linearly across multiple cores using a lock-free OpenMP architecture.

## Theoretical Framework & Algorithm Complexity

This engine bridges theoretical combinatorial optimization with bare-metal runtime execution, evaluating the 2D Rectangular Packing Problem through a strictly Data-Oriented Design (DOD) lens. The architecture explicitly separates the stochastic state-space exploration (The Player) from the deterministic geometric placement (The Dealer) to maximize CPU throughput.

1. Deterministic Placement (Bottom-Left Heuristic)
- **Concept**: A geometric decoder that guarantees topologically valid states by placing pieces as close to the bottom-left corner of the container as possible without overlapping.
- **Complexity**: Traditional Object-Oriented implementations traverse a 2D grid using nested spatial loops, resulting in a placement complexity of $O(W \times H)$ per place, which severely bottlenecks stochastic evaluations.
- **Implementation**: AegisHeuristics abandons 2D matrices in favor of 1D aligned bitboards mapped via a Struct-of-Arrays (SoA). Collision detection and gap finding are performed using bitwise AND operations alongside the __builtin_ctzll (Count Trailing Zeros) hardware instrinsic. This collapses the spatial search complexity to $O(1)$ per row evaluated, allowing the engine to process millions of placements per second while entirely eliminating branch mispredictions.

2. Stochastic Exploration (Simulated Annealing)
- **Concept**: A probabilistic meta-heuristic that navigates the vast permutation space of piece sequences and 90-degree rotations. It utilizes a geometric cooling schedule, accepting occasionally worse states to successfully escape local minima.
- **Complexity**: The execution time complexity scales at $O(I \times N)$, where $I$ represents the total cooling iterations and $N$ the number of pieces per instance.
- **Implementation**: The search space is navigated using an "embarrassingly parallel" architecture driven by OpenMP. To prevent the severe thread contention typical of concurrent stochastic algorithms, the engine replaces the standard std::mt19937 generator with a custom, thread-local Xorshift64 PRNG that remains bound to the L1 cache. Furthermore, the Metropolis acceptance criterion bypasses the high latency of std::exp() by directly manipulating IEEE 754 floating-point bits (fast_exp), achieving perfect linear scaling across all logical cores with zero mutex locks.

## Architecture & Technologies

AegisHeuristics is engineered with a strict adherence to Data-Oriented Design (DOD) principles, focusing on extreme cache efficiency, bitwise arithmetic, and lock-free concurrency. The system completely decouples the raw C++ combinatorial engine from the analytical reporting pipeline.

- **Language**: C++23
- **Memory Architecture (Data Locality)**: To prevent L1 cache misses and heap fragmentation, the engine abandons traditional Object-Oriented std::vector matrices during the "hot path". Instead, it utilizes 1D stack-allocated std::array configurations mapped via a Structure of Arrays (SoA). All spatial mutations rely exclusively on 64-bit integer masks and hardware intrinsics (__builtin_ctzll).
- **Concurrency Model**: Implements an "embarrassingly parallel" OpenMP architecture. By utilizing lock-free reductions and a custom, thread-local Xorshift64 PRNG, the meta-heuristic scales linearly across all logical CPU cores without suffering from mutex contention or false sharing.
- **Build System**: CMake (3.20+) configured for Unity Builds (Single Compilation Unit) to maximize compiler inline optimizations. It utilizes FetchContent for seamless, self-contained dependency management without polluting the host OS.
- **Testing & Profilling**: Integrated with GoogleTest (GTest) to rigorously assert deterministic bitboard boundaries (preventing floating pieces and stack overflows) and Google Benchmark to extract nanosecond-level CPU latency metrics.
- **Telemetry & Visualization**: A decoupled analytical pipeline. The C++ engine emits pure JSON telemetry which is ingested by a Python environment to render statistical convergence graphs, ultimately compiled into academic scientific reports using Typst.

## Build & Run Instructions.

The project is optimized for a native Linux terminal workflow. The build system utilizes FetchContent to automatically download, compile, and statically link Google Test and Google Benchmark, ensuring a self-contained compilation process.

### Prerequisites
- A C++20 compatible compiler with robust hardware intrinsic support (e.g., GCC 15.2.0+).
- CMake (Version 3.20 or higher)
- OpenMP (Version 4.5+)
- Python 3.10+ (for data processing)

### Building the Project

1. **Clone the repository**:
Open your terminal and navigate to the directory where you want to clone the repository.

SSH:
```bash
git clone [https/ssh_url]
```
2. **Compiling the Monolitic Engine**:

Open your terminal at the root of the repository and execute the following commands to configure and build the highly optimized release binaries. The -Release flag is critical to enable -O3 and hardware-specific instruction sets.

```bash
# Create the build directory
mkdir -p build && cd build

# Configure CMake for Release
cmake -DCMAKE_BUILD_TYPE=Release ..

# Compile the core engine, tests, benchmarks, and the main runner using all available CPU cores
cmake --build . -j $(nproc)
```

3. **Mathematical Validation (GTest)**:

Before running the stochastic engine, strictly verify the integrity of the bitwise geometric decoder. These tests assert the absence of floating pieces, validate rotation mechanics, and ensure L1 cache boundaries are strictly respected to prevent buffer overflows.

```bash
# Run the test suite via CTest
ctest --output-on-failure
```

4. **Micro-Benchmarking (GBenchmark)**:

Extract pure CPU latency metrics by stress-testing the Bottom-Left bitwise evaluator. This command generates the telemetry JSON required by the Python analytical pipeline.

```bash
# Run the benchmark and dump the telemetry
./benchmarks/aegis_benchmarks --benchmark_format=json > benchmark_telemetry.json
```

5. **Executing the Simulated Annealing Solver**:

Launch the primary optimization engine to solve the problem instances defined in the PDF dataset. The engine will automatically detect and utilize all available hardware threads.

```bash
# Execute the parallel solver
./src/aegis_run
```

6. **Telemetry & Visualization Pipeline**:

To translate the raw JSON benchmark data into scientific graphs, utilize the decoupled Python pipeline. It is highly recommended to operate within a virtual environment.

```bash
# Navigate to the scripts directory
cd ../scripts

# Set up and activate the Python virtual environment
python3 -m venv venv
source venv/bin/activate

# Install the required data science libraries
pip install pandas matplotlib seaborn

# Run the visualization parser
python3 parse_benchmarks.py
```

The script will ingest benchmark_telemetry.json and output vector graphics detailing the engine's convergence rates and execution speed.

## Author

Sergio "PhillipNoir" Gonzalez -- Computer Engineering Student and low-level software enthusiast.
This project was engineered as a requirement for the Analysis and Design of Algorithms course at Universidad Autonoma Metropolitana (UAM). If this architecture has helped you understand data-oriented design or high-performance heuristics, please consider giving the repository a star or buying me a coffee:

https://coff.ee/phillipnoir