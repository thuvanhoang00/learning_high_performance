#!/bin/bash

# --- Configuration ---
BUILD_DIR="build"
EXECUTABLE_NAME="Mat_bench_profile"
# ---------------------

# 1. Safety First: Stop script on any error
set -e

echo "=========================================="
echo "🚀 Starting High-Performance Build Pipeline"
echo "=========================================="

# 2. Check if 'perf' is installed
if ! command -v perf &> /dev/null; then
    echo "❌ Error: 'perf' is not installed. Install with: sudo apt-get install linux-tools-common linux-tools-generic"
    exit 1
fi

# 3. Create Build Directory (Clean build if needed, but incremental is faster)
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# 4. Run CMake
# -DCMAKE_BUILD_TYPE=RelWithDebInfo is CRITICAL for meaningful profiling
echo "🔧 Configuring CMake (RelWithDebInfo)..."
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# 5. Compile
echo "🔨 Compiling..."
# -j$(nproc) uses all available CPU cores for faster compilation
make -j$(nproc)

# 6. Verify Executable Exists
if [ ! -f "./$EXECUTABLE_NAME" ]; then
    echo "❌ Error: Executable '$EXECUTABLE_NAME' not found!"
    exit 1
fi

echo "=========================================="
echo "📊 Running perf record..."
echo "=========================================="

# 7. Run Perf Record
# -g : Captures the "Call Graph" (stack trace). This tells you WHO called the slow function.
# -F 99 : Samples at 99Hz (avoids lock-step sampling with code loops).
# --  : Separator between perf flags and your command.
perf stat \
  -e task-clock,context-switches,cpu-migrations,page-faults,cache-misses \
  -e cycles,instructions,branches,branch-misses \
  -e L1-dcache-loads,L1-dcache-load-misses \
  -r 3 \
   ./$EXECUTABLE_NAME

echo "✅ Done."