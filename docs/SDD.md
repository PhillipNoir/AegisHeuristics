# Aegis Heuristics - Software Design Document

## Executive Summary

### Project Objective

AegisHeuristics is a high-performance optimization engine engineered to solve specific instances of the 2D rectangular packing problem, focusing on non-guillotineable layouts where items can be rotated by 90 degrees. Specifically targeting bounding boxes of $20 \times 20$ units (category 1) and $40 \times 15$ units (category 2), the system is designed to maximize packing density while virtually eliminating memory latency. The primary goal is to achieve an optimal mathematical balance between raw execution speed and global optimality to dominate academic benchmarking criteria.

### Core Architecture & DOD

Drawing on low-level engine architecture principles, the system abandons traditional object-oriented programming in favor of pure Data-Oriented Design (DOD). The packing environment is represented through cache-aligned, one-dimensional 64-bit integer arrays (Bitboards), guaranteeing absolute spatial locality in the CPU's L1 cache. Piece attributes (dimensions and pre-computed rotation masks) are strictly isolated using a Structure of Arrays (SoA) layout. This mechanical design eliminates branch mispredictions and enables $O(1)$ collision detection and implicit skyline tracking via bitwise compiler intrinsics. To maximize hardware utilization, the project utilizes a CMake-configured Unity Build, forcing the compiler to treat the entire codebase as a single translation unit for aggressive, cross-function inlining.

### Algorithmic Engine

The solver hybridizes a deterministic packing procedure that preserves bottom-left stability with a Simulated Annealing (SA) meta-heuristic. The architecture enforces a strict separation of concerns to maintain CPU throughput:

- **The dealer (Bottom-Left)**: A rigid, bitwise placement routine ensuring 100% physically valid topological states at minimal cycle cost.

- **The player (Simulated Annealing)**: A state-space explorer utilizing geometric cooling and high-speed mutation of topological indices and boolean rotation vectors.

### Concurrency & Ecosystem Pipeline

To achieve linear horizontal scaling, the engine employs an "Embarrassingly Parallel" execution model via OpenMP. By relying on lock-free reduction clauses and thread-local instances of a lightweight Xorshift64 PRNG, the system explores combinatorial branches concurrently without mutex contention. The development lifecycle is reinforced by mathematical validation (Google Test) and rigorous micro-benchmarking (Google Benchmark). Performance telemetry is systematically parsed via Python scripts and natively integrated into Typst scientific reports, ensuring an automated, data-backed optimization pipeline.

## Base Architecture

### Container Representation (Bitboards)

The packing grid is modeled using a single-dimensional array of unsigned 64-bit integers, where each index directly represents a discrete row ($Y$-coordinate) and individual bits represent columns ($X$-coordinate).

```cpp
alignas(64) uint64_t board[MAX_Y] = {0};
```

To maximize cache efficiency and prevent lane splits during execution, the array is explicitly aligned to a 64-byte boundary using the alignas(64) specifier. Because modern CPU architectures fetch data from main memory into L1 cache in 64-byte cache lines, a single memory transaction loads exactly 8 consecutive rows (64 bits per row) directly into the processor's fastest memory layer.

Given that the target boundaries are strictly capped at a maximum width of 20 units for Category 1 and 40 units for Category 2, a standard uint64_t provides more than enough horizontal capacity native to the registers. Columns beyond the active width are permanently masked out as occupied to prevent out-of-bounds placements without requiring runtime conditional checks.

### Piece Property Storage (SoA)

AegisHeuristics strictly avoids the traditional Array of Structures (AoS) pattern to eliminate cache pollution. Packing attributes are decoupled into separate, tightly packed parallel arrays:

```cpp
namespace Pieces {
    int widths[MAX_PIECES];
    int heights[MAX_PIECES];
    uint64_t normal_masks[MAX_PIECES][MAX_Y_PIECES];
    uint64_t rotated_masks[MAX_PIECES][MAX_Y_PIECES];
}
```

During the evaluation of the Bottom-Left placement loop, the engine only streams the specific bitwise masks required for collision testing. Storing metadata like raw integers alongside the masks would waste cache line capacity with unneeded data. Furthermore, all $90^{\circ}$ rotations are fully resolved and cached as static bit masks during the initialization phase, converting a complex geometric rotation into a simple array pointer swap during the Simulated Annealing phase.

### Low-Level Collision Detection & Spatial Instrinsics

Collision checking between a candidate piece configuration and the current state of the board is condensed into an unrolled loop of bitwise AND operations. A piece fits at a designated row offset if and only if the bitwise intersection between the board layers and the pre-computed piece masks yields zero for all overlapping rows:

```cpp
// O(1) Collision Check per row
if ((board[target_y + row] & piece_mask[row]) != 0){
    // Collision detected, stop checking   
}
```

Instead of tracking spatial layout constraints via an explicit, dynamically managed skyline array—which introduces branch mispredictions and variable pointer chasing—the empty spaces within the container are queried implicitly. The engine employs hardware-accelerated compiler intrinsics, specifically __builtin_ctzll (Count Trailing Zeros), to scan rows for available placement windows in a single CPU clock cycle. This turns the entire layout terrain into an implicit, zero-overhead bitwise skyline.

### Translation Unit Optimization (Unity Build)

To eliminate function call overhead and maximize register reuse within the intensely clustered meta-heuristic loops, the system utilizes a CMake-driven Unity Build compilation architecture.

```cmake
set_target_properties(AegisHeuristics PROPERTIES UNITY_BUILD ON)
```

By forcing CMake to inject all .cpp implementations into a single monolithic compilation stream, the entire codebase is processed as one unified translation unit. This empowers the compiler to perform aggressive, global cross-file optimization, completely bypasses the limitations of traditional link-time optimizations, and ensures that critical helper functions are fully inlined directly into the execution path of the Simulated Annealing loops.

## Algorithmic Engine

### Algorithmic Decoupling (Dealer & Player)

To prevent branching overhead and ensure clean, maintainable, and highly optimized code, AegisHeuristics enforces a strict decoupling of roles within the core execution loop. The solving process is split into an independent state generator and a deterministic state evaluator:

- **The Dealer (Bottom-Left)**: Acts as a pure, deterministic decoder. It takes the sequence and orientation vectors generated by the Player, reads the pre-computed bit masks from the SoA structure, and packs the items sequentially into the bitboard using rigid topological rules. Because the Dealer guarantees that every produced layout is structurally valid, the Player never spends clock cycles validating or repairing illegal states.

- **The Player (Simulated Annealing)**: Operates strictly on a meta-topological level. It does not understand geometric space, boundaries, or collisions. It treats the problem as an abstract combinatorial sequence, manipulating a permutation vector of piece indices and an independent boolean vector of orientation flags.

### The Dealer: Bitwise Bottom-Left Placement

The packing routine evaluates the incoming sequence of items one by one. For each piece, the engine determines the absolute lowest and leftmost available coordinate window on the bitboard where the piece can be placed without intersecting existing shapes.

```Plaintext
Algorithm 1: Bitwise Bottom-Left Placement Routine
Input: Piece Sequence Vector S, Rotation Vector R
Output: Container Height H

1: Reset board array to 0
2: For each piece index i in S:
3:    Get mask_array matching orientation R[i] from Pieces SoA
4:    Set target_y = 0
5:    Set target_x = 0
6:    Loop rows from target_y to find the first row where:
7:       (board[target_y + row] & (mask_array[row] << target_x)) == 0
8:    If collision occurs horizontally, shift target_x and re-evaluate
9:    Commit piece to board: board[target_y + row] |= (mask_array[row] << target_x)
10: Return highest occupied index Y as Container Height H
```

By leveraging bitwise shifting (<< target_x), shifting an entire piece across the horizontal axis takes a fraction of a nanosecond, allowing the layout engine to scan hundreds of placement windows within a single execution path.

### The Player: Simulated Annealing & Mutation Loci

The state-space exploration is driven by a localized Simulated Annealing routine. The current state is completely encapsulated by a structure containing the permutation array and the rotation bitset.To generate a neighboring state ($S'$), the engine invokes a high-speed mutation function that relies on the thread-local Xorshift64 generator. The function applies one of two lightweight topological alterations to avoid breaking data locality:
- **Sequence Swap**: Two distinct indices within the permutation array are randomly selected and swapped. This alters the order in which the Dealer processes the layout, fundamentally shifting the spatial priority.
- **Orientation Flip**: A single index within the rotation bitset is randomly selected and inverted (changing a piece from normal to its $90^{\circ}$ cached alternative).

### Cooling Schedule & Fast Mathematical Acceptance

The engine drives convergence using a clean geometric cooling schedule, where the temperature diminishes at a constant fractional rate:

$$T_{k+1} = \alpha * T_k \text{ where } \alpha \in [0.95, 0.99]$$

When a mutation yields a worse solution ($\Delta E > 0$, meaning a layout with a greater container height), the engine computes the Metropolis acceptance probability $P = e^{-\Delta E / T}$. To protect the execution pipeline from the high clock-cycle latency of the standard std::exp() library function, the calculation utilizes a fast bit-cast approximation:

```cpp
// Fast bit-cast exponential approximation via IEEE 754 exponent manipulation
float fast_exp(float val) {
    // Exploits the linear relationship of the exponent bits in a 32-bit float
    float result;
    int32_t i = (int32_t)(12102203.0f * val + 1065353216.0f);
    std::memcpy(&result, &i, sizeof(float)); // Strict aliasing compliant zero-overhead transfer
    return result;
}
```

## Concurrency & Randomness

### PRNG Blottleneck Elimination (Thread-Local Xorshift64)

In stochastic optimization algorithms like Simulated Annealing, the Pseudo-Random Number Generator (PRNG) is called millions of times per second. Using standard library generators like std::mt19937 (Mersenne Twister) introduces a massive 2.5 KB internal state footprint, which inherently pollutes the L1 cache and bottlenecks execution.

To maintain the strict Data-Oriented Design philosophy, AegisHeuristics implements a custom Xorshift64 generator.

- **State Minimization**: The entire PRNG state is reduced to a single 64-bit integer, fitting perfectly inside a native CPU register.

- **Instruction-Level Parallelism**: Generating the next random number requires only three fundamental ALU operations (bitwise shifts and XORs).

- **Thread Isolation**: To guarantee thread safety without synchronization locks, the state is declared using the thread_local keyword. This forces the compiler to instantiate a distinct, independent copy of the PRNG state in the exclusive memory pool of each hardware thread.

```cpp
// Isolated entropy state for each execution thread
thread_local uint64_t xorshift_state;

// Ultra-fast, register-bound PRNG
inline uint64_t fast_rand() {
    uint64_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return xorshift_state = x;
}
```

### Embarrassingly Parallel Execution Model (OpenMP)

The engine achieves absolute linear horizontal scaling by employing an "Embarrassingly Parallel" architecture. Because Simulated Annealing relies on random topological walks to escape local optima, there is no mathematical requirement for threads to share state during the exploration phase.

Using OpenMP, the system dynamically queries the host OS for the maximum number of available logical cores and deploys an independent Simulated Annealing trajectory (a "Player") on each core. Each thread is initialized with a deterministic, unique seed based on its thread ID, ensuring maximum divergence in the search space.

### Lock-Free State Aggregation

The most common performance killer in multi-threaded C++ applications is thread contention caused by std::mutex locking. If multiple threads constantly lock a global variable to update the "best found height," the CPU cores will idle in a waiting queue, destroying throughput.

AegisHeuristics entirely circumvents mutexes by leveraging OpenMP's reduction clause.

```cpp
int global_best_height = 999999;
int total_independent_runs = 1000;

// OpenMP handles thread pooling, work-stealing, and lock-free aggregation
#pragma omp parallel for reduction(min:global_best_height)
for (int i = 0; i < total_independent_runs; ++i) {
    
    // Seed isolation based on thread topology
    xorshift_state = BASE_SEED + omp_get_thread_num() + i;
    
    // Execute full SA isolated inside the thread
    int local_best = execute_simulated_annealing();
    
    // The reduction clause safely handles the minimum comparison 
    // at the end of the parallel region without mutex locking.
    if (local_best < global_best_height) {
        global_best_height = local_best;
    }
}
```

By keeping all combinatorial mutations and placement evaluations strictly isolated within the thread's local execution context, and only merging the scalar results at the very end of the lifecycle, the engine guarantees 100% CPU utilization across any multi-core architecture.

## Toolchain & Development Pipeline

### Build Automation & Compiler Aggression (CMAKE)

The engine's compilation lifecycle is orchestrated entirely through CMake. To fulfill the architectural requirement of a monolithic execution path, the CMakeLists.txt is configured to force a Unity Build (UNITY_BUILD ON), combining all implementations into a single translation unit for maximum cross-function inlining.

Targeting modern C++ compilers (GCC/Clang) typical in Linux-based development environments, the build pipeline enforces extreme optimization flags to squeeze every cycle out of the hardware:
- **-03**: Maximum optimization level for execution speed.
- **march=native**: Unlocks architecture-specific instruction sets (such as BMI1/BMI2 for hardware-level bit manipulation).
- **ffast-math**: Relaxes strict IEEE 754 compliance to accelerate the Simulated Annealing exponential acceptance probability.
-**fopenmp**: Enables OpenMP support for parallel execution.

### Micro-Benchmarking (Google Benchmark)

Because AegisHeuristics is built around Data-Oriented Design (DOD) principles, preventing performance regressions is critical. The pipeline integrates Google Benchmark to isolate and measure the raw throughput of the Bottom-Left bitwise placement routine. The suite is configured to sample execution times down to the nanosecond and CPU cycle level, guaranteeing that the $O(1)$ collision detection maintains its theoretical performance bounds under varying loads.

### Algorithmic Correctness (Google Test)

A stochastic meta-heuristic is only as reliable as its deterministic decoder. To ensure the "Dealer" never produces an illegal state, the project employs Google Test (GTest). The unit testing suite strictly validates the integrity of the Structure of Arrays (SoA), verifying that the static pre-computation of $90^{\circ}$ piece rotations and the bitwise boundary shifts behave flawlessly before any Simulated Annealing loop is executed.

### Internal API Documentation (Doxygen)

Despite the monolithic nature of the codebase, maintainability is preserved through inline documentation. All core structures, bitboard operations, and mathematical approximations are annotated using standard Doxygen tags. This allows the toolchain to automatically generate a clean, navigable API reference, acting as a single source of truth for the development team.

## Telemetry & Reporting Pipeline

### Telemetry Ingestion (Google Benchmark JSON)

While tabular formats (CSV) provide lightweight data logging, AegisHeuristics standardizes entirely on Google Benchmark's native JSON output format. This hierarchical structure is critical for High-Performance Computing (HPC) telemetry, as it captures not only raw execution latency (CPU time, real-time, and iteration counts) but also vital hardware context. The JSON payload automatically records target architecture specifications, dynamic CPU frequency scaling, and L1/L2/L3 cache limits during the run. This ensures that any performance variability in the bitwise operations can be strictly correlated with hardware-level states.

### Python Visualization Scripts

The raw telemetry is programmatically ingested by a dedicated suite of Python scripts (scripts/parse_benchmarks.py). Utilizing data analysis libraries, the pipeline parses the JSON payloads to automatically generate high-fidelity performance charts. These visual artifacts plot the Simulated Annealing convergence curve (Container Height vs. Iteration/Temperature), hardware scaling efficiency, and execution microsecond variance. The scripts output optimized SVG and PNG assets directly into the docs/reports/ directory.

### Scientific Report Integration (Typst)

To maintain a pristine, reproducible, and highly modular documentation standard, the final stage of the pipeline relies on Typst for scientific typesetting. The engine's Typst templates are configured to dynamically link to the Python-generated performance charts and benchmark datasets. This zero-friction integration guarantees that every time the C++ engine is recompiled and benchmarked, the resulting mathematical models, convergence graphs, and architectural documentation are instantly updated and rendered into a publication-ready PDF, reflecting the absolute latest state of the monolithic engine.