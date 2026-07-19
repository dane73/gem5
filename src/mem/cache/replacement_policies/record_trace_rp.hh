#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_RECORD_TRACE_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_RECORD_TRACE_RP_HH__

#include <fstream>
#include <memory>
#include <string>

#include "mem/cache/replacement_policies/base.hh"

namespace gem5
{

struct RecordTraceRPParams;

namespace replacement_policy
{

/**
 * Wrapper policy that records the block address of every cache access
 * to a trace file, while delegating all replacement decisions to a
 * sub-policy. The recorded trace can be replayed by BeladyRP.
 *
 * Entries carry the sub-policy's replacement data directly; this policy
 * keeps no per-entry state of its own.
 */
class RecordTraceRP : public Base
{
  protected:
    /** Sub-policy making the actual replacement decisions. */
    Base *const recorderPolicy;

    /** Cache line size, used to compute packets' block addresses. */
    const unsigned blkSize;

    /** Resolved path of the trace file. */
    std::string filename;

    /** Trace output stream. */
    std::ofstream cacheTrace;

  public:
    PARAMS(RecordTraceRP);
    RecordTraceRP(const Params &p);

    void invalidate(
        const std::shared_ptr<ReplacementData> &replacement_data) override;
    void touch(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void touch(const std::shared_ptr<ReplacementData> &replacement_data)
        const override;
    void reset(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void reset(const std::shared_ptr<ReplacementData> &replacement_data)
        const override;
    ReplaceableEntry *
    getVictim(const ReplacementCandidates &candidates) const override;
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_RECORD_TRACE_RP_HH__
