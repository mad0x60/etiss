/*
        @copyright
        <pre>
        Copyright 2018 Infineon Technologies AG
        This file is part of ETISS tool, see <https://github.com/tum-ei-eda/etiss>.
        </pre>
        @author Chair of Electronic Design Automation, TUM
        @version 0.1
*/

#include <mutex>
#include <cstdint>

// Statistics structure - matches what JITStatsCollector uses
struct JITTranslationStats {
    uint64_t fastJitBlocks;
    uint64_t optimizingJitBlocks;
    uint64_t cacheNextHits;
    uint64_t cacheBranchHits;
    uint64_t cacheMisses;
    uint64_t blocksOptimized;
    uint64_t blocksSwitched;
    bool fastJitEnabled;
};

// Global stats storage (thread-safe access)
static JITTranslationStats g_jitStats = {0, 0, 0, 0, 0, 0, 0, false};
static std::mutex g_jitStatsMutex;

// Function to update stats (called from Translation.cpp and can be called from plugin)
extern "C" void updateJITTranslationStats(
    uint64_t fastJit, uint64_t optJit, uint64_t next, 
    uint64_t branch, uint64_t miss, uint64_t optimized, 
    uint64_t switched, bool fastEnabled) 
{
    std::lock_guard<std::mutex> lock(g_jitStatsMutex);
    g_jitStats.fastJitBlocks = fastJit;
    g_jitStats.optimizingJitBlocks = optJit;
    g_jitStats.cacheNextHits = next;
    g_jitStats.cacheBranchHits = branch;
    g_jitStats.cacheMisses = miss;
    g_jitStats.blocksOptimized = optimized;
    g_jitStats.blocksSwitched = switched;
    g_jitStats.fastJitEnabled = fastEnabled;
}

// Function to get stats (called from JITStatsCollector plugin)
extern "C" void getJITTranslationStats(
    uint64_t* fastJit, uint64_t* optJit, uint64_t* next, 
    uint64_t* branch, uint64_t* miss, uint64_t* optimized, 
    uint64_t* switched, bool* fastEnabled)
{
    std::lock_guard<std::mutex> lock(g_jitStatsMutex);
    if (fastJit) *fastJit = g_jitStats.fastJitBlocks;
    if (optJit) *optJit = g_jitStats.optimizingJitBlocks;
    if (next) *next = g_jitStats.cacheNextHits;
    if (branch) *branch = g_jitStats.cacheBranchHits;
    if (miss) *miss = g_jitStats.cacheMisses;
    if (optimized) *optimized = g_jitStats.blocksOptimized;
    if (switched) *switched = g_jitStats.blocksSwitched;
    if (fastEnabled) *fastEnabled = g_jitStats.fastJitEnabled;
}

