#include "mem/cache/tags/partitioning_policies/sector_hint.hh"

namespace gem5
{

namespace partitioning_policy
{

uint64_t
SectorHintPartitionManager::readPacketPartitionID(PacketPtr pkt) const
{
    if (!pkt->req) {
        return NON_TEMPORAL_PARTITION_ID;
    }

    auto ext = pkt->req->getExtension<SectorHintExtension>();

    // Requests the core did not classify -- page table walks and anything
    // else that does not come from readMem/writeMem -- must not be able to
    // allocate in the protected partition, so they default to non-temporal.
    // That also matches the old two-run scheme, where every access outside
    // the temporal set was cacheable in the nt run.
    return ext ? ext->getPartitionID() : NON_TEMPORAL_PARTITION_ID;
}

} // namespace partitioning_policy

} // namespace gem5
