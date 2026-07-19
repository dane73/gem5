#include "mem/cache/replacement_policies/record_trace_rp.hh"

#include "base/logging.hh"
#include "base/output.hh"
#include "params/RecordTraceRP.hh"

namespace gem5
{

namespace replacement_policy
{

RecordTraceRP::RecordTraceRP(const Params &p)
    : Base(p), recorderPolicy(p.recorder_policy), blkSize(p.cache_line_size)
{
    fatal_if(recorderPolicy == nullptr,
             "The recorder replacement policy must be instantiated");

    filename = simout.resolve(p.trace_file);
    inform("Recording cache trace in %s", filename);
    cacheTrace.open(filename, std::ios::out | std::ios::trunc);
    fatal_if(!cacheTrace.is_open(), "Could not open cache trace file %s",
             filename);
}

void
RecordTraceRP::invalidate(
    const std::shared_ptr<ReplacementData> &replacement_data)
{
    recorderPolicy->invalidate(replacement_data);
}

void
RecordTraceRP::touch(const std::shared_ptr<ReplacementData> &replacement_data,
                     const PacketPtr pkt)
{
    cacheTrace << std::hex << pkt->getBlockAddr(blkSize) << std::endl;
    recorderPolicy->touch(replacement_data, pkt);
}

void
RecordTraceRP::touch(
    const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("Cannot record a trace access without a packet");
}

void
RecordTraceRP::reset(const std::shared_ptr<ReplacementData> &replacement_data,
                     const PacketPtr pkt)
{
    cacheTrace << std::hex << pkt->getBlockAddr(blkSize) << std::endl;
    recorderPolicy->reset(replacement_data, pkt);
}

void
RecordTraceRP::reset(
    const std::shared_ptr<ReplacementData> &replacement_data) const
{
    panic("Cannot record a trace access without a packet");
}

ReplaceableEntry *
RecordTraceRP::getVictim(const ReplacementCandidates &candidates) const
{
    return recorderPolicy->getVictim(candidates);
}

std::shared_ptr<ReplacementData>
RecordTraceRP::instantiateEntry()
{
    return recorderPolicy->instantiateEntry();
}

} // namespace replacement_policy
} // namespace gem5
