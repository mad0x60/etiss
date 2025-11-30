/*
        @copyright
        <pre>
        Copyright 2018 Infineon Technologies AG
        This file is part of ETISS tool, see <https://github.com/tum-ei-eda/etiss>.
        </pre>
        @author Chair of Electronic Design Automation, TUM
        @version 0.1
*/

#include "JITStatsCollector.h"
#include <cmath>

// External function declarations (defined in src/JITStats.cpp, compiled into libETISS.so)
extern "C" {
    void getJITTranslationStats(uint64_t*, uint64_t*, uint64_t*, uint64_t*, uint64_t*, uint64_t*, uint64_t*, bool*);
}

void JITStatsCollector::printStats()
{
    JITTranslationStats stats;
    getJITTranslationStats(
        &stats.fastJitBlocks,
        &stats.optimizingJitBlocks,
        &stats.cacheNextHits,
        &stats.cacheBranchHits,
        &stats.cacheMisses,
        &stats.blocksOptimized,
        &stats.blocksSwitched,
        &stats.fastJitEnabled
    );
    
    uint64_t totalBlocks = stats.fastJitBlocks + stats.optimizingJitBlocks;
    uint64_t totalCacheHits = stats.cacheNextHits + stats.cacheBranchHits;
    uint64_t totalCacheLookups = totalCacheHits + stats.cacheMisses;
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "========================================\n";
    ss << "ETISS JIT Compilation Statistics\n";
    ss << "========================================\n\n";
    
    ss << "JIT Configuration:\n";
    ss << "  Fast JIT enabled: " << (stats.fastJitEnabled ? "Yes" : "No") << "\n\n";
    
    ss << "Block Compilation Statistics:\n";
    ss << "  Total blocks compiled: " << totalBlocks << "\n";
    if (stats.fastJitEnabled) {
        ss << "  - Fast JIT compiled: " << stats.fastJitBlocks;
        if (totalBlocks > 0) {
            double pct = (100.0 * stats.fastJitBlocks) / totalBlocks;
            ss << " (" << pct << "%)";
        }
        ss << "\n";
    }
    
    ss << "  - Optimizing JIT compiled: " << stats.optimizingJitBlocks;
    if (totalBlocks > 0) {
        double pct = (100.0 * stats.optimizingJitBlocks) / totalBlocks;
        ss << " (" << pct << "%)";
    }
    ss << "\n\n";
    
    if (stats.fastJitEnabled && stats.fastJitBlocks > 0) {
        ss << "Hybrid JIT Benefits:\n";
        ss << "  Blocks optimized in background: " << stats.blocksOptimized << "\n";
        ss << "  Blocks switched to optimized: " << stats.blocksSwitched << "\n";
        double optimizationRate = (100.0 * stats.blocksOptimized) / stats.fastJitBlocks;
        ss << "  Optimization success rate: " << optimizationRate << "%\n";
        if (stats.blocksOptimized > 0) {
            double switchRate = (100.0 * stats.blocksSwitched) / stats.blocksOptimized;
            ss << "  Switch-to-optimized rate: " << switchRate << "%\n";
        }
        ss << "\n";
    }
    
    ss << "Block Cache Statistics:\n";
    ss << "  Total cache lookups: " << totalCacheLookups << "\n";
    ss << "  - Cache hits (sequential): " << stats.cacheNextHits;
    if (totalCacheLookups > 0) {
        double pct = (100.0 * stats.cacheNextHits) / totalCacheLookups;
        ss << " (" << pct << "%)";
    }
    ss << "\n";
    
    ss << "  - Cache hits (branch): " << stats.cacheBranchHits;
    if (totalCacheLookups > 0) {
        double pct = (100.0 * stats.cacheBranchHits) / totalCacheLookups;
        ss << " (" << pct << "%)";
    }
    ss << "\n";
    
    ss << "  - Cache misses: " << stats.cacheMisses;
    if (totalCacheLookups > 0) {
        double pct = (100.0 * stats.cacheMisses) / totalCacheLookups;
        ss << " (" << pct << "%)";
    }
    ss << "\n";
    
    if (totalCacheLookups > 0) {
        double hitRate = (100.0 * totalCacheHits) / totalCacheLookups;
        ss << "  Cache hit rate: " << hitRate << "%\n";
    }
    ss << "========================================\n";
    
    if (printOnScreen_) {
        std::cout << ss.str() << std::endl;
    }
    
    // Write to file
    std::ofstream outFile(outPath_);
    if (outFile.is_open()) {
        outFile << ss.str();
        outFile.close();
        if (!printOnScreen_) {
            std::cout << "JIT statistics written to: " << outPath_ << std::endl;
        }
    } else {
        etiss::log(etiss::WARNING, "Failed to write JIT statistics to file: " + outPath_);
    }
}
