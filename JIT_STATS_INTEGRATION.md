# JIT Statistics Collector Integration Guide

This document explains how to integrate the JITStatsCollector plugin with the Translation system.

## Overview

The JITStatsCollector plugin tracks:
- Fast JIT vs Optimizing JIT compilation counts
- Block cache hit/miss statistics  
- Hybrid JIT benefits (background optimization, hot-swapping)

## Required Modifications

### 1. Add Statistics Counters to Translation Class

In `etiss/include/etiss/Translation.h`, add to the private section (around line 211):

```cpp
    // JIT Statistics (always enabled)
    uint64_t fastJitBlocks_;
    uint64_t optimizingJitBlocks_;
    uint64_t blocksOptimized_;
    uint64_t blocksSwitched_;
```

And in the public section, add a method:

```cpp
    // Get current statistics
    void getStats(uint64_t &fastJit, uint64_t &optJit, uint64_t &optimized, 
                  uint64_t &switched) const {
        fastJit = fastJitBlocks_;
        optJit = optimizingJitBlocks_;
        optimized = blocksOptimized_;
        switched = blocksSwitched_;
    }
```

### 2. Initialize Counters in Translation Constructor

In `etiss/src/Translation.cpp`, in the constructor initialization list (around line 141):

```cpp
    , fastJitBlocks_(0)
    , optimizingJitBlocks_(0)
    , blocksOptimized_(0)
    , blocksSwitched_(0)
```

### 3. Track Fast JIT Compilations

In `Translation::getBlock()`, when fast JIT compilation succeeds (around line 439):

```cpp
                nbl->hasOptimized = false;
                fastJitBlocks_++;  // ADD THIS LINE
```

### 4. Track Optimizing JIT Compilations

In `Translation::getBlock()`, when optimizing JIT is used (around line 471):

```cpp
        nbl = new BlockLink(block.startindex_, block.endaddress_, execBlock, lib);
        optimizingJitBlocks_++;  // ADD THIS LINE
```

### 5. Track Blocks Switched to Optimized Version

In `Translation::getBlockFast()`, when optimized version is activated (around line 299):

```cpp
                    if (iterbl->hasOptimized && iterbl->execBlock != iterbl->optimizedExecBlock) {
                        iterbl->execBlock = iterbl->optimizedExecBlock;
                        iterbl->jitlib = iterbl->optimizedJitLib;
                        blocksSwitched_++;  // ADD THIS LINE
```

### 6. Track Background Optimizations

In `OptimizationManager::updateBlockWithOptimizedVersion()` (around line 851):

```cpp
        block->hasOptimized = true;
        // We need to increment blocksOptimized_ counter here
        // Since this is in OptimizationManager, we need to pass Translation reference
        // OR use the global update function
```

### 7. Update Statistics Periodically

In `Translation::~Translation()` destructor (around line 152), add:

```cpp
// Include the header
#include "bare_etiss_processor/JITStatsCollector.h"

// In destructor:
    // Export final statistics
    bool fastEnabled = (fastJit_ != nullptr);
    #if ETISS_TRANSLATOR_STAT
    updateJITTranslationStats(fastJitBlocks_, optimizingJitBlocks_, 
                             next_count_, branch_count_, miss_count_,
                             blocksOptimized_, blocksSwitched_, fastEnabled);
    #else
    updateJITTranslationStats(fastJitBlocks_, optimizingJitBlocks_,
                             0, 0, 0, blocksOptimized_, blocksSwitched_, fastEnabled);
    #endif
```

**Alternative**: Instead of modifying Translation destructor, you can call `updateJITTranslationStats()` periodically during execution or access Translation stats through CPUCore.

### 8. Enable ETISS_TRANSLATOR_STAT for Cache Statistics

To track cache hits/misses, you need to build ETISS with `-DETISS_TRANSLATOR_STAT=1` or add it to CMakeLists.txt.

## Plugin Usage

Add to `main.cpp` (similar to TimeTracker):

```cpp
#include "JITStatsCollector.h"

// In main(), after loadIniJIT:
if (etiss::cfg().get<bool>("jit_stats.enable", false)) {
    bool enable_print = etiss::cfg().get<bool>("jit_stats.print", true);
    std::string prefix = etiss::cfg().get<std::string>("etiss.output_path_prefix", ".");
    std::string out_path = etiss::cfg().get<std::string>("jit_stats.out_path", 
                                                          prefix + "/JITStats.log");
    cpu->addPlugin(std::shared_ptr<etiss::Plugin>(new JITStatsCollector(enable_print, out_path)));
}
```

## Configuration

Add to INI file:

```ini
[jit_stats]
enable=true
print=true
out_path=./JITStats.log
```

## Notes

- Statistics are collected throughout simulation
- Final statistics are printed when plugin is destroyed (after simulation ends)
- Cache statistics require ETISS_TRANSLATOR_STAT to be enabled
- Thread-safe access using mutexes

