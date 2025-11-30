# JIT Statistics Linker Fix

## Problem

The linker error occurred because:
- `Translation.cpp` (compiled into `libETISS.so`) was calling `updateJITTranslationStats()`
- The function was defined in `JITStatsCollector.cpp` (only compiled into `bare_etiss_processor` executable)
- When linking `libETISS.so`, the linker couldn't find the function

## Solution

Created `src/JITStats.cpp` which:
- Defines `updateJITTranslationStats()` - called by `Translation.cpp`
- Defines `getJITTranslationStats()` - called by the plugin to read stats
- Gets automatically compiled into `libETISS.so` (via `FILE(GLOB *.cpp)` in `src/CMakeLists.txt`)

Now both `Translation.cpp` and the plugin can access the statistics:
- `Translation.cpp` → writes to stats via `updateJITTranslationStats()`
- `JITStatsCollector.cpp` → reads from stats via `getJITTranslationStats()`

## Files Changed

1. **Created**: `src/JITStats.cpp` - Shared statistics storage and functions
2. **Updated**: `src/bare_etiss_processor/JITStatsCollector.cpp` - Now uses `getJITTranslationStats()` instead of accessing globals
3. **Updated**: `src/bare_etiss_processor/JITStatsCollector.h` - Removed duplicate extern declarations

## Verification

The compilation should now succeed:
```bash
cd build_dir
cmake .. -DCMAKE_BUILD_TYPE=Release -DETISS_TRANSLATOR_STAT=ON
make -j$(nproc)
```

No more undefined reference errors!

