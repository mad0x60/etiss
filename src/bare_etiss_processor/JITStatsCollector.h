/*
        @copyright
        <pre>
        Copyright 2018 Infineon Technologies AG
        This file is part of ETISS tool, see <https://github.com/tum-ei-eda/etiss>.
        </pre>
        @author Chair of Electronic Design Automation, TUM
        @version 0.1
*/

#ifndef JIT_STATS_COLLECTOR_H
#define JIT_STATS_COLLECTOR_H

#include <fstream>
#include <iostream>
#include <iomanip>
#include "etiss/ETISS.h"

// Statistics structure - matches what Translation exports
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

/**
 * @brief Plugin to collect and report statistics about JIT compilation and block caching.
 *
 * Tracks:
 * - Fast JIT compilations
 * - Optimizing JIT compilations  
 * - Cache hits (next, branch, miss)
 * - Blocks optimized in background
 * - Blocks that switched from fast to optimized version
 *
 * @param [Optional] print: Print stats on screen
 * @param [Optional] out_path: Location of final stats file
 */
class JITStatsCollector : public etiss::CoroutinePlugin
{
  public:
    // ctor
    JITStatsCollector(bool print = true, std::string out_path = "./JITStats.log")
        : CoroutinePlugin(), printOnScreen_(print), outPath_(out_path)
    {
    }

    // dtor
    ~JITStatsCollector() { printStats(); }

    etiss::int32 execute()
    {
        // This plugin just collects stats, doesn't need per-block execution
        return etiss::RETURNCODE::NOERROR;
    }

    std::string _getPluginName() const { return std::string("JITStatsCollector"); }

    void init(ETISS_CPU *cpu, ETISS_System *system, etiss::CPUArch *arch)
    {
        this->cpu_ = cpu;
        this->system_ = system;
        this->arch_ = arch;
    }

    void cleanup()
    {
        cpu_ = nullptr;
        system_ = nullptr;
        arch_ = nullptr;
    }

    void executionEnd(int32_t code)
    {
        // Don't print stats here - Translation destructor hasn't run yet to export final stats.
        // Stats will be printed in destructor after Translation exports final stats.
    }

  private:
    ETISS_CPU *cpu_;
    ETISS_System *system_;
    etiss::CPUArch *arch_;

    bool printOnScreen_;
    std::string outPath_;

    void printStats();
};

#endif // JIT_STATS_COLLECTOR_H
