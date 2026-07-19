/**
 * @file
 * Declaration of Belady's optimal replacement policy, which replays a
 * recorded cache access trace to evict the entry whose next use is
 * furthest in the future.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__

#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <unordered_map>

#include "mem/cache/replacement_policies/base.hh"

namespace gem5
{

struct BeladyRPParams;

namespace replacement_policy
{

/**
 * Belady's optimal replacement policy. Replays a cache access trace
 * (one block address per line, hex, recorded e.g. by RecordTraceRP) and
 * victimizes the candidate whose next use lies furthest in the future,
 * or one that is never used again.
 *
 * Every touch()/reset() consumes exactly one trace entry, mirroring how
 * the trace was recorded; any divergence panics.
 */
class BeladyRP : public Base
{
  protected:
    struct BeladyReplData : ReplacementData
    {
        /** Whether the entry holds usable data. */
        bool valid;

        /** Block address held by this entry; only meaningful while
         * valid is set. */
        Addr addr;

        /** Default constructor. Invalidate data. */
        BeladyReplData() : valid(false), addr(0) {}
    };

    /** Trace file stream; only used to fill addresses on construction. */
    std::ifstream cacheTrace;

    /** Per block address: queue of trace indices of its future uses. */
    std::unordered_map<Addr, std::deque<size_t>> addresses;

    /** Block size, used to compute packets' block addresses. */
    uint64_t cacheLineSize;

    /** Index of the next trace entry to be consumed. */
    size_t currIdx;

    /** Next use of address, or MaxAddr if it is never used again. */
    size_t get_index(Addr address) const;

    /** Consume the trace entry of address; panics on out-of-order use. */
    void pop_index(Addr address);

  public:
    PARAMS(BeladyRP);
    BeladyRP(const Params &p);
    ~BeladyRP() = default;

    /**
     * Invalidate replacement data. The slot loses its address identity,
     * so the next reset() consumes a trace entry for the new address.
     */
    void invalidate(
        const std::shared_ptr<ReplacementData> &replacement_data) override;

    /**
     * Update replacement data on a hit: consumes one trace entry.
     */
    void touch(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void touch(const std::shared_ptr<ReplacementData> &replacement_data)
        const override;

    /**
     * Reset replacement data on insertion: consumes one trace entry and
     * binds the slot to the inserted block address. Must only be called
     * for empty or invalidated slots.
     */
    void reset(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void reset(const std::shared_ptr<ReplacementData> &) const override;

    /**
     * Find replacement victim: an invalid entry if one exists, otherwise
     * the candidate whose next use is furthest away or never comes.
     */
    ReplaceableEntry *
    getVictim(const ReplacementCandidates &candidates) const override;

    /** Instantiate a replacement data entry. */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__
