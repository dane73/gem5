#include "mem/cache/replacement_policies/belady_rp.hh"

#include <cassert>
#include <memory>

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
    if (!cacheTrace.is_open()) {
        panic("Cache Trace File");
    }
    Addr tmp;
    size_t idx = 0;
    while (cacheTrace >> std::hex >> tmp) {
        if (tmp == MaxAddr) {
            panic("MaxAddr found");
        }
        addresses[tmp].push_back(idx);
        idx++;
    }
    cacheTrace.close();
}

size_t
BeladyRP::get_index(Addr address) const
{
    auto it = addresses.find(address);
    if (it == addresses.end()) {
        panic("Address not exist in trace");
    }
    if (it->second.empty()) {
        return MaxAddr;
    }
    return it->second.front();
}

void
BeladyRP::pop_index(Addr address)
{
    auto it = addresses.find(address);
    if (it == addresses.end()) {
        panic("Address not exist in trace");
    }
    if (it->second.empty()) {
        panic("This address does not have any uses left");
    }

    if (it->second.front() != currIdx) {
        panic("Reihenfolge");
    }
    addresses[address].pop_front();
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

    if (replData->addr != pkt_blk_addr) {
        std::cout << "not same addresses: " << (replData->addr) << "|"
                  << pkt_blk_addr << std::endl;
        panic("touch not same address");
    }

    replData->addr = pkt_blk_addr;
    pop_index(pkt_blk_addr);
    replData->valid = true;
}

void
BeladyRP::touch(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("wrong def\n");
}

void
BeladyRP::reset(const std::shared_ptr<ReplacementData> &replacement_data,
                const PacketPtr pkt)
{
    assert(replacement_data);

    auto replData = std::static_pointer_cast<BeladyReplData>(replacement_data);
    Addr pkt_blk_addr = pkt->getBlockAddr(cacheLineSize);

    if (pkt->getAddr() != pkt->getBlockAddr(cacheLineSize)) {
        panic("Address is not a block addr");
    }

    if (replData->addr != pkt_blk_addr || !replData->init) {
        if (!replData->init) {
            replData->init = true;
        }
        replData->addr = pkt_blk_addr;
        pop_index(pkt_blk_addr);
    }
    replData->valid = true;
}

void
BeladyRP::reset(const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("wrong reset\n");
}

ReplaceableEntry *
BeladyRP::getVictim(const ReplacementCandidates &candidates) const
{
    // There must be at least one replacement candidate
    assert(candidates.size() > 0);

    ReplaceableEntry *victim = candidates[0];

    // throw out the first invalid cache block found
    for (auto &candidate : candidates) {
        auto candidate_replData = std::static_pointer_cast<BeladyReplData>(
            candidate->replacementData);
        if (!candidate_replData->valid) {
            return candidate;
        }
    }

    auto victim_replData =
        std::static_pointer_cast<BeladyReplData>(victim->replacementData);
    size_t victimNextUsage = get_index(victim_replData->addr);

    // if not invalid cache block exists, throw out the cache block with the
    // furthes next usage, or that cache block which wont be used again.
    for (auto &candidate : candidates) {
        auto candidate_replData = std::static_pointer_cast<BeladyReplData>(
            candidate->replacementData);
        victim_replData =
            std::static_pointer_cast<BeladyReplData>(victim->replacementData);

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
