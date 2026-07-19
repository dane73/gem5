#include "mem/cache/replacement_policies/belady_rp.hh"

#include <cassert>
#include <memory>

#include "base/logging.hh"
#include "params/BeladyRP.hh"

namespace gem5
{

namespace replacement_policy
{

BeladyRP::BeladyRP(const Params &p)
    : Base(p),
      cacheTrace(p.cache_trace),
      cacheLineSize(p.cache_line_size),
      currIdx(0)
{
    fatal_if(!cacheTrace.is_open(), "Could not open cache trace file %s",
             p.cache_trace);

    Addr tmp;
    size_t idx = 0;
    while (cacheTrace >> std::hex >> tmp) {
        fatal_if(tmp == MaxAddr,
                 "Cache trace contains the reserved address MaxAddr");
        addresses[tmp].push_back(idx);
        idx++;
    }
    cacheTrace.close();
}

size_t
BeladyRP::get_index(Addr address) const
{
    auto it = addresses.find(address);
    panic_if(it == addresses.end(), "Address %#x does not exist in the trace",
             address);
    if (it->second.empty()) {
        return MaxAddr;
    }
    return it->second.front();
}

void
BeladyRP::pop_index(Addr address)
{
    auto it = addresses.find(address);
    panic_if(it == addresses.end(), "Address %#x does not exist in the trace",
             address);
    panic_if(it->second.empty(), "Address %#x has no uses left in the trace",
             address);
    panic_if(
        it->second.front() != currIdx,
        "Trace replay out of order: expected trace index %d, but the next "
        "use of address %#x is index %d",
        currIdx, address, it->second.front());

    it->second.pop_front();
    currIdx++;
}

void
BeladyRP::invalidate(const std::shared_ptr<ReplacementData> &replacement_data)
{
    assert(replacement_data);

    std::static_pointer_cast<BeladyReplData>(replacement_data)->valid = false;
}

void
BeladyRP::touch(const std::shared_ptr<ReplacementData> &replacement_data,
                const PacketPtr pkt)
{
    assert(replacement_data);

    auto replData = std::static_pointer_cast<BeladyReplData>(replacement_data);
    Addr pkt_blk_addr = pkt->getBlockAddr(cacheLineSize);

    panic_if(replData->addr != pkt_blk_addr,
             "touch() for address %#x on an entry holding address %#x",
             pkt_blk_addr, replData->addr);

    pop_index(pkt_blk_addr);
    replData->valid = true;
}

void
BeladyRP::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("BeladyRP requires the packet-aware variant of touch()");
}

void
BeladyRP::reset(const std::shared_ptr<ReplacementData> &replacement_data,
                const PacketPtr pkt)
{
    assert(replacement_data);

    auto replData = std::static_pointer_cast<BeladyReplData>(replacement_data);
    Addr pkt_blk_addr = pkt->getBlockAddr(cacheLineSize);

    panic_if(pkt->getAddr() != pkt_blk_addr,
             "reset() packet address %#x is not block-aligned",
             pkt->getAddr());

    // reset() must only be called on insertion into an empty or
    // invalidated slot (see the replacement policy contract)
    panic_if(replData->valid,
             "reset() on a slot that was not invalidated (holds address %#x)",
             replData->addr);

    replData->addr = pkt_blk_addr;
    pop_index(pkt_blk_addr);
    replData->valid = true;
}

void
BeladyRP::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("BeladyRP requires the packet-aware variant of reset()");
}

ReplaceableEntry *
BeladyRP::getVictim(const ReplacementCandidates &candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    // Prefer an invalid entry if one exists
    for (auto &candidate : candidates) {
        auto candidate_replData = std::static_pointer_cast<BeladyReplData>(
            candidate->replacementData);
        if (!candidate_replData->valid) {
            return candidate;
        }
    }

    ReplaceableEntry *victim = candidates[0];
    size_t victimNextUsage = get_index(
        std::static_pointer_cast<BeladyReplData>(victim->replacementData)
            ->addr);

    // Otherwise victimize the entry whose next use is furthest away, or
    // one that is never used again
    for (auto &candidate : candidates) {
        auto candidate_replData = std::static_pointer_cast<BeladyReplData>(
            candidate->replacementData);

        size_t nextUsage = get_index(candidate_replData->addr);
        // case the address will not be loaded again
        if (nextUsage == MaxAddr) {
            return candidate;
        }
        if (victimNextUsage < nextUsage) {
            victim = candidate;
            victimNextUsage = nextUsage;
        }
    }

    return victim;
}

std::shared_ptr<ReplacementData>
BeladyRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new BeladyReplData());
}

} // namespace replacement_policy
} // namespace gem5
