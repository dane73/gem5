/**
 * @file
 * Partition manager driven by per-reference locality hints from the core.
 *
 * The core tags every data request with the partition the reference belongs
 * to. Only the victim search is restricted to that partition's ways; the tag
 * lookup stays global. This is what a sector cache does: the sector ID steers
 * allocation, never the hit check, so a reference still hits on a line that
 * the other sector brought in. Emulating the split with two separate runs
 * cannot express that and turns every such cross-sector reuse into a miss.
 */

#ifndef __MEM_CACHE_TAGS_PARTITIONING_POLICIES_SECTOR_HINT_HH__
#define __MEM_CACHE_TAGS_PARTITIONING_POLICIES_SECTOR_HINT_HH__

#include <cstdint>
#include <memory>

#include "base/extensible.hh"
#include "mem/cache/tags/partitioning_policies/partition_manager.hh"
#include "mem/packet.hh"
#include "mem/request.hh"
#include "params/SectorHintPartitionManager.hh"

namespace gem5
{

namespace partitioning_policy
{

/** Partition holding the references the analysis marked temporal. */
static constexpr uint64_t TEMPORAL_PARTITION_ID = 0;
/** Partition holding everything else, including all streaming traffic. */
static constexpr uint64_t NON_TEMPORAL_PARTITION_ID = 1;

/**
 * Carries the partition of a reference from the core down to the cache.
 */
class SectorHintExtension : public Extension<Request, SectorHintExtension>
{
  public:
    SectorHintExtension() = default;
    explicit SectorHintExtension(uint64_t id) : partitionId(id) {}

    std::unique_ptr<ExtensionBase>
    clone() const override
    {
        return std::make_unique<SectorHintExtension>(*this);
    }

    uint64_t
    getPartitionID() const
    {
        return partitionId;
    }

  private:
    uint64_t partitionId = NON_TEMPORAL_PARTITION_ID;
};

class SectorHintPartitionManager : public PartitionManager
{
  public:
    PARAMS(SectorHintPartitionManager);
    using PartitionManager::PartitionManager;

    uint64_t readPacketPartitionID(PacketPtr pkt) const override;
};

} // namespace partitioning_policy

} // namespace gem5

#endif // __MEM_CACHE_TAGS_PARTITIONING_POLICIES_SECTOR_HINT_HH__
