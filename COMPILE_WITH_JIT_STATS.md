# Compiling ETISS with JIT Statistics Collector

This guide explains how to compile ETISS with the JIT Statistics Collector plugin enabled.

## Overview

The JIT Statistics Collector tracks:
- Fast JIT vs Optimizing JIT compilation counts
- Block cache hit/miss statistics (requires ETISS_TRANSLATOR_STAT)
- Hybrid JIT benefits (background optimization, hot-swapping)

## Integration Summary

The following files have been modified/created:

### Created Files:
- `src/bare_etiss_processor/JITStatsCollector.h` - Plugin header
- `src/bare_etiss_processor/JITStatsCollector.cpp` - Plugin implementation

### Modified Files:
- `include/etiss/Translation.h` - Added statistics counters and callback
- `src/Translation.cpp` - Added statistics tracking and export
- `src/bare_etiss_processor/main.cpp` - Added plugin initialization
- `src/bare_etiss_processor/CMakeLists.txt` - Added JITStatsCollector.cpp to build

## Compilation Steps

### Option 1: Build with Cache Statistics (Recommended)

To enable cache statistics tracking, compile with `ETISS_TRANSLATOR_STAT`:

```bash
cd /path/to/etiss
mkdir -p build_dir
cd build_dir
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DETISS_TRANSLATOR_STAT=ON \
    -DCMAKE_INSTALL_PREFIX=/path/to/install
make -j$(nproc)
make install
```

**Note**: Use `-DETISS_TRANSLATOR_STAT=ON` (or `=1`) to enable. The option is now properly integrated into CMakeLists.txt.

### Option 2: Build without Cache Statistics

If you don't need cache statistics, you can omit the flag (cache counts will be 0):

```bash
cd /path/to/etiss
mkdir -p build_dir
cd build_dir
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/path/to/install
make -j$(nproc)
make install
```

### Option 3: Enable by Default

The option is already in `CMakeLists.txt`. To enable it by default, you can modify line 64:

```cmake
option(ETISS_TRANSLATOR_STAT "Enable translator statistics" ON)  # Change OFF to ON
```

Then compile normally:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Configuration

### Enable the Plugin

Add to your ETISS configuration file (e.g., `ETISS.ini`):

```ini
[jit_stats]
enable=true
print=true
out_path=./JITStats.log
```

Or via command line arguments:
```bash
./bare_etiss_processor \
    -ijit_stats.enable=true \
    -ijit_stats.print=true \
    -ijit_stats.out_path=./JITStats.log \
    -i<your_config>.ini
```

### Configuration Options

- `jit_stats.enable`: Enable the statistics collector (default: false)
- `jit_stats.print`: Print statistics to console (default: true)  
- `jit_stats.out_path`: Path to output file (default: `./JITStats.log`)

## Usage Example

### Basic Usage

```bash
# Run with JIT statistics enabled
./bare_etiss_processor \
    -ijit_stats.enable=true \
    -ijit_stats.print=true \
    -i<your_program>.ini
```

### With Hybrid JIT

```bash
# Run with Fast JIT + Optimizing JIT
./bare_etiss_processor \
    -ijit_stats.enable=true \
    -ijit_stats.print=true \
    -ijit.type=GCCJIT \
    -ijit.fast_type=TCCJIT \
    -i<your_program>.ini
```

### Output Only to File

```bash
# Don't print to console, only write to file
./bare_etiss_processor \
    -ijit_stats.enable=true \
    -ijit_stats.print=false \
    -ijit_stats.out_path=./my_stats.log \
    -i<your_program>.ini
```

## Output Format

The plugin outputs statistics like:

```
========================================
ETISS JIT Compilation Statistics
========================================

JIT Configuration:
  Fast JIT enabled: Yes

Block Compilation Statistics:
  Total blocks compiled: 1234
  - Fast JIT compiled: 1000 (81.04%)
  - Optimizing JIT compiled: 234 (18.96%)

Hybrid JIT Benefits:
  Blocks optimized in background: 850
  Blocks switched to optimized: 820
  Optimization success rate: 85.00%
  Switch-to-optimized rate: 96.47%

Block Cache Statistics:
  Total cache lookups: 50000
  - Cache hits (sequential): 45000 (90.00%)
  - Cache hits (branch): 4000 (8.00%)
  - Cache misses: 1000 (2.00%)
  Cache hit rate: 98.00%
========================================
```

## Troubleshooting

### Cache Statistics Show Zero

**Problem**: Cache statistics (next hits, branch hits, misses) all show 0.

**Solution**: Ensure you compiled with `-DETISS_TRANSLATOR_STAT=1`:
```bash
cmake .. -DETISS_TRANSLATOR_STAT=1
```

### Plugin Not Loading

**Problem**: Statistics are not collected.

**Check**:
1. Verify `JITStatsCollector.cpp` is compiled (check build logs)
2. Ensure configuration has `jit_stats.enable=true`
3. Check for compilation errors

### Compilation Errors

**Common issues**:
- Missing `<functional>` header - should be automatically included
- Missing mutex - standard library, should be available
- Link errors - ensure `JITStatsCollector.cpp` is in CMakeLists.txt

## Statistics Explained

### Block Compilation Statistics
- **Fast JIT compiled**: Blocks initially compiled with fast JIT (e.g., TCC)
- **Optimizing JIT compiled**: Blocks compiled directly with optimizing JIT (fallback)

### Hybrid JIT Benefits
- **Blocks optimized in background**: Blocks that were optimized asynchronously
- **Blocks switched to optimized**: Blocks that actually used the optimized version
- **Optimization success rate**: Percentage of fast JIT blocks that got optimized
- **Switch-to-optimized rate**: Percentage of optimized blocks that were switched

### Cache Statistics (requires ETISS_TRANSLATOR_STAT)
- **Cache hits (sequential)**: Blocks found via `next` pointer (sequential execution)
- **Cache hits (branch)**: Blocks found via `branch` pointer (branch target)
- **Cache misses**: Blocks not in cache, requiring translation

## Files Modified

1. **Translation.h** - Added statistics counters
2. **Translation.cpp** - Added tracking at key points:
   - Fast JIT compilation (line ~444)
   - Optimizing JIT compilation (line ~475)
   - Block switching (line ~310)
   - Background optimization (via callback)
   - Statistics export (destructor)

3. **main.cpp** - Added plugin initialization
4. **CMakeLists.txt** - Added source file

## Dependencies

- C++14 or later
- Standard library mutex support
- Standard library functional support (for callbacks)

The plugin has no external dependencies beyond what ETISS already requires.
