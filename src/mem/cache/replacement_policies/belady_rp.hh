/**
 * @file
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "proto/protoio.hh"

namespace gem5
{

struct BeladyRPParams;

namespace replacement_policy
{

class BeladyRP : public Base
{
  protected:
    // enum class IdxState {
    //     Uninit,
    //     Init,
    //     Finished,
    // };

    struct BeladyReplData : ReplacementData
    {
        bool valid;
        Addr addr;

        // this is needed since addr gets initialzed with 0, which is an
        // address that could actually get used.
        // However, it is nessessary to differentiate between unitialized
        // and initalized ways
        bool init;

        /**
         * Default constructor. Invalidate data.
         */
        BeladyReplData() : valid(false), addr(0), init(false) {}
    };

    std::ifstream cacheTrace;
    std::unordered_map<Addr, std::deque<size_t>> addresses;
    uint64_t cacheLineSize;

    size_t currIdx;
    size_t get_index(Addr address) const;
    void pop_index(Addr address);

  public:
    typedef BeladyRPParams Params;
    BeladyRP(const Params &p);
    ~BeladyRP() = default;

    /**
     * Invalidate replacement data to set it as the next probable victim.
     * Sets its last touch tick as the starting tick.
     *
     * @param replacement_data Replacement data to be invalidated.
     */
    void invalidate(
        const std::shared_ptr<ReplacementData> &replacement_data) override;

    /**
     * Touch an entry to update its replacement data.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be touched.
     */
    void touch(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void touch(const std::shared_ptr<ReplacementData> &replacement_data)
        const override;

    /**
     * Reset replacement data. Used when an entry is inserted.
     * Sets its last touch tick as the current tick.
     *
     * @param replacement_data Replacement data to be reset.
     */
    void reset(const std::shared_ptr<ReplacementData> &replacement_data,
               const PacketPtr pkt) override;
    void reset(const std::shared_ptr<ReplacementData> &) const override;

    /**
     * Find replacement victim using LRU timestamps.
     *
     * @param candidates Replacement candidates, selected by indexing policy.
     * @return Replacement entry to be replaced.
     */
    ReplaceableEntry *
    getVictim(const ReplacementCandidates &candidates) const override;

    /**
     * Instantiate a replacement data entry.
     *
     * @return A shared pointer to the new replacement data.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

} // namespace replacement_policy
} // namespace gem5

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_BELADY_RP_HH__
