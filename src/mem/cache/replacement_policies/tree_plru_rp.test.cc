/**
 * Copyright (c) 2025 The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtest/gtest.h>

#include "mem/cache/replacement_policies/tree_plru_rp.hh"
#include "params/TreePLRURP.hh"

class TreePLRUTestF : public ::testing::Test
{
  public:
    std::shared_ptr<gem5::replacement_policy::TreePLRU> rp;
    int numLeaves = 8;

    TreePLRUTestF(int num_entries = 8)
    {
        gem5::TreePLRURPParams params;
        params.eventq_index = 0;
        numLeaves = num_entries;
        params.num_leaves = numLeaves;
        rp = std::make_shared<gem5::replacement_policy::TreePLRU>(params);
    }
};

TEST_F(TreePLRUTestF, InstantiatedEntry)
{
    const auto repl_data = rp->instantiateEntry();
    ASSERT_NE(repl_data, nullptr);
}

/// Test that if there is one candidate and it is invalid, it will be
/// the victim
TEST_F(TreePLRUTestF, GetVictim1Candidate)
{
    gem5::ReplaceableEntry entry;
    entry.replacementData = rp->instantiateEntry();
    gem5::ReplacementCandidates candidates;
    candidates.push_back(&entry);
    ASSERT_EQ(rp->getVictim(candidates), &entry);

    rp->invalidate(entry.replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entry);
}

/// Fixture that tests victimization
class TreePLRUVictimizationTestF : public TreePLRUTestF
{
  protected:
    /// The entries being victimized
    std::vector<gem5::ReplaceableEntry> entries;

    /// The entries, in candidate form
    gem5::ReplacementCandidates candidates;

  public:
    void
    instantiateAllEntries(void)
    {
        for (auto &entry : entries) {
            entry.replacementData = rp->instantiateEntry();
            candidates.push_back(&entry);
        }
    }

    TreePLRUVictimizationTestF(int num_entries = 8)
        : TreePLRUTestF(num_entries), entries(numLeaves)
    {
        instantiateAllEntries();
    }
};

/// Test resetting no entries. The tree's nodes should all have values of 0,
/// pointing toward entry A at index 0.
TEST_F(TreePLRUVictimizationTestF, GetVictimNoReset)
{
    ASSERT_EQ(rp->getVictim(candidates), &entries[0]);
}

/// Test that when all entries are invalid the first candidate will always be
/// selected, regardless of the order of the invalidations.
TEST_F(TreePLRUVictimizationTestF, GetVictimAllInvalid)
{
    auto expected_victim = &entries.front();

    /// At this point all tree nodes should have values of 0, as no entries
    /// have ever been reset. The tree points toward the first entry.
    ///    ____0____
    ///  __0__   __0__
    /// _0_ _0_ _0_ _0_
    /// A B C D E F G H

    /// Since all candidates are already invalid, nothing changes if we
    /// invalidate all of them again.
    for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
        rp->invalidate((*it)->replacementData);
    }
    ASSERT_EQ(rp->getVictim(candidates), expected_victim);
}

/// Test only resetting one entry

/// Test that if the entry at index 0 is the most recently used, the entry at
/// index 4 will be the victim.
/// The tree looks like this after candidate A is reset:
///    ____1____
///  __1__   __0__
/// _1_ _0_ _0_ _0_
/// A B C D E F G H
/// The tree points to candidate E as the victim.

TEST_F(TreePLRUVictimizationTestF, GetVictimSingleResetLeftmost)
{
    rp->reset(entries[0].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
}

/// Reset entry H and entry A will be victimized.
/// The tree after reset:
///    ____0____
///  __0__   __0__
/// _0_ _0_ _0_ _0_
/// A B C D E F G H

TEST_F(TreePLRUVictimizationTestF, GetVictimSingleResetRightmost)
{
    rp->reset(entries[7].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[0]);
}

/// Reset entry B and entry E will be victimized.
/// The tree after reset:
///    ____1____
///  __1__   __0__
/// _0_ _0_ _0_ _0_
/// A B C D E F G H

TEST_F(TreePLRUVictimizationTestF, GetVictimSingleResetMiddle)
{
    rp->reset(entries[1].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
}

/// Entries A, B, E, and F are reset, in that order. The victimized entry
/// should be C at index 2.
/// The tree after resetting entry A
///    ____1____
///  __1__   __0__
/// _1_ _0_ _0_ _0_
/// A B C D E F G H
///
/// The tree after resetting entry B
///    ____1____
///  __1__   __0__
/// _0_ _0_ _0_ _0_
/// A B C D E F G H
///
/// The tree after resetting entry E
///    ____0____
///  __1__   __1__
/// _0_ _0_ _1_ _0_
/// A B C D E F G H
///
/// The tree after resetting entry F
///    ____0____
///  __1__   __1__
/// _0_ _0_ _0_ _0_
/// A B C D E F G H

TEST_F(TreePLRUVictimizationTestF, GetVictimHalfReset)
{
    rp->reset(entries[0].replacementData);
    rp->reset(entries[1].replacementData);
    rp->reset(entries[4].replacementData);
    rp->reset(entries[5].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[2]);
}

/// Reset all entries once, starting from the leftmost side of the tree. The
/// victimized entry should be A.

TEST_F(TreePLRUVictimizationTestF, GetVictimAllReset)
{
    for (auto &entry : entries) {
        rp->reset(entry.replacementData);
    }

    ASSERT_EQ(rp->getVictim(candidates), &entries[0]);
}

/// Reset all entries twice, starting from the leftmost side, then from the
/// rightmost side. The victimized entry should be the rightmost entry,
/// H, at index 7.

TEST_F(TreePLRUVictimizationTestF, GetVictimAllTwiceReset)
{
    for (auto &entry : entries) {
        rp->reset(entry.replacementData);
    }

    for (auto entry = entries.rbegin(); entry != entries.rend(); entry++) {
        rp->reset((*entry).replacementData);
    }

    ASSERT_EQ(rp->getVictim(candidates), &entries[7]);
}

/// `touch()` and `reset()` should have the same behavior for the TreePLRU
/// replacement policy. This unit test checks this.
TEST_F(TreePLRUVictimizationTestF, CheckTouchResetSame)
{
    /// We touch/reset the same entries as in GetVictimHalfReset, but swap
    /// out a varying number of the resets for touches. The victim should still
    /// remain the same.
    std::vector<int> indices{0, 1, 4, 5};
    for (int i = 1; i < 4; i++) {
        for (int j = 0; j < i; j++) {
            rp->touch(entries[indices[j]].replacementData);
        }
        for (int j = i; j < 4; j++) {
            rp->reset(entries[indices[j]].replacementData);
        }
        ASSERT_EQ(rp->getVictim(candidates), &entries[2]);
        for (auto &entry : entries) {
            rp->reset(entry.replacementData);
        }
    }
}

/// Test that when there is at least a single invalid entry, it will be
/// selected during the victimization

TEST_F(TreePLRUVictimizationTestF, GetVictimOneInvalid)
{
    for (auto &entry : entries) {
        /// Validate all entries to start from a clean state
        for (auto &entry : entries) {
            rp->reset(entry.replacementData);
        }

        /// Set one of the entries as invalid
        rp->invalidate(entry.replacementData);

        ASSERT_EQ(rp->getVictim(candidates), &entry);
    }
}

/// Instantiate enough entries to fill two trees, then check that making
/// changes in one tree doesn't affect the other
TEST_F(TreePLRUVictimizationTestF, TestTwoTrees)
{
    /// we store entries from both trees in `entries`, so resize it to
    /// accomodate all entries
    entries.resize(16);
    /// ensure that the first `candidates` vector has the correct memory
    /// locations
    for (int i = 0; i < 8; i++) {
        candidates[i] = &entries[i];
    }

    gem5::ReplacementCandidates second_candidates(8);
    for (int i = 8; i < 16; i++) {
        entries[i].replacementData = rp->instantiateEntry();
        second_candidates[i - 8] = &entries[i];
    }

    /// if the trees are separate (as they should be), then the victim entry
    /// should be entries[4]. If not, entries[8] will be selected.
    rp->reset(entries[0].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
    ASSERT_NE(rp->getVictim(candidates), &entries[8]);

    /// If the entries are all (incorrectly) in the same tree, then entries[7]
    /// will be selected.
    rp->reset(entries[8].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
    ASSERT_NE(rp->getVictim(candidates), &entries[7]);

    ASSERT_EQ(rp->getVictim(second_candidates), &entries[12]);
    ASSERT_NE(rp->getVictim(second_candidates), &entries[7]);
}

TEST_F(TreePLRUVictimizationTestF, TestMixedResetInvalidate)
{
    /// If the entry is correctly invalidated, the entry at index 5 will be
    /// selected.
    rp->reset(entries[0].replacementData);
    rp->invalidate(entries[5].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[5]);

    /// Check that there aren't any issues with calling reset and invalidate
    /// on the same entry
    rp->reset(entries[1].replacementData);
    rp->invalidate(entries[1].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[1]);
}

/// Build a candidate list holding only the entries at the given indices, in
/// order. Cache partitioning passes such a subset of a set's ways to
/// getVictim(): the tree still spans the whole set, but only these ways may be
/// victimised.
gem5::ReplacementCandidates
subset(std::vector<gem5::ReplaceableEntry> &entries,
       const std::vector<int> &indices)
{
    gem5::ReplacementCandidates out;
    for (int i : indices) {
        out.push_back(&entries[i]);
    }
    return out;
}

/// With a filtered candidate list the tree may point at a leaf that is not a
/// candidate. The victim must then be the correct leaf among the ones that
/// are, not an out-of-range access (the bug this path fixes).
///
/// Fresh tree, all nodes 0, so the unconstrained victim would be entry A:
///    ____0____
///  __0__   __0__
/// _0_ _0_ _0_ _0_
/// A B C D E F G H
///
/// Restricting the candidates to the right half {E,F,G,H} makes the walk skip
/// the empty left subtree; with both right-subtree children populated the 0
/// bits then steer to the leftmost of them, E at index 4.
TEST_F(TreePLRUVictimizationTestF, GetVictimPartitionRightHalf)
{
    auto candidates = subset(entries, {4, 5, 6, 7});
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
}

/// The other half {A,B,C,D}, after entry A is made MRU:
///    ____1____
///  __1__   __0__
/// _1_ _0_ _0_ _0_
/// A B C D E F G H
/// Unconstrained this points at E (index 4); restricted to the left half the
/// right subtree is empty, so the walk stays left, the node-1 bit sends it
/// right to the {C,D} pair, and the node-4 bit picks C at index 2.
TEST_F(TreePLRUVictimizationTestF, GetVictimPartitionLeftHalf)
{
    rp->reset(entries[0].replacementData);
    /// The unconstrained victim is outside the partition.
    ASSERT_EQ(rp->getVictim(candidates), &entries[4]);
    auto left = subset(entries, {0, 1, 2, 3});
    ASSERT_EQ(rp->getVictim(left), &entries[2]);
}

/// A candidate subset equal to the whole set exercises the unfiltered fast
/// path and must reproduce the unconstrained victim exactly (a partition that
/// owns every way behaves like no partition).
TEST_F(TreePLRUVictimizationTestF, GetVictimPartitionFullSetUnchanged)
{
    rp->reset(entries[0].replacementData);
    auto all = subset(entries, {0, 1, 2, 3, 4, 5, 6, 7});
    ASSERT_EQ(all.size(), candidates.size());
    ASSERT_EQ(rp->getVictim(all), rp->getVictim(candidates));
    ASSERT_EQ(rp->getVictim(all), &entries[4]);
}

/// An invalid entry inside the partition still wins: invalidate() points the
/// tree at entry G (index 6), and with G in the candidate subset the walk
/// follows those bits straight to it.
TEST_F(TreePLRUVictimizationTestF, GetVictimPartitionInvalidInSubset)
{
    rp->invalidate(entries[6].replacementData);
    auto candidates = subset(entries, {4, 5, 6, 7});
    ASSERT_EQ(rp->getVictim(candidates), &entries[6]);
}

class SmallTreePLRUVictimizationTestF : public TreePLRUVictimizationTestF
{
  public:
    SmallTreePLRUVictimizationTestF() : TreePLRUVictimizationTestF(2) {}
};

class LargeTreePLRUVictimizationTestF : public TreePLRUVictimizationTestF
{
  public:
    LargeTreePLRUVictimizationTestF() : TreePLRUVictimizationTestF(512) {}
};

TEST_F(SmallTreePLRUVictimizationTestF, TestSmallTree)
{
    /// check that the test has been set up correctly
    ASSERT_EQ(entries.size(), 2);
    ASSERT_EQ(candidates.size(), 2);

    /// check that resetting one entry will cause the other to be selected
    rp->reset(entries[0].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[1]);

    for (auto &entry : entries) {
        rp->reset(entry.replacementData);
    }
    rp->reset(entries[1].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[0]);

    /// check invalidate
    rp->invalidate(entries[1].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[1]);
}

TEST_F(LargeTreePLRUVictimizationTestF, TestLargeTree)
{
    /// check that the test has been set up correctly
    ASSERT_EQ(entries.size(), 512);
    ASSERT_EQ(candidates.size(), 512);

    rp->reset(entries[0].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[256]);

    rp->invalidate(entries[511].replacementData);
    ASSERT_EQ(rp->getVictim(candidates), &entries[511]);
}

typedef TreePLRUTestF TreePLRUDeathTest;

class OneLeafTreePLRUDeathTestF : public TreePLRUVictimizationTestF
{
  public:
    OneLeafTreePLRUDeathTestF() : TreePLRUVictimizationTestF(1) {}
};

TEST_F(TreePLRUDeathTest, InvalidateNull)
{
    ASSERT_DEATH(rp->invalidate(nullptr), "");
}

TEST_F(TreePLRUDeathTest, ResetNull)
{
    ASSERT_DEATH(rp->reset(nullptr), "");
}

TEST_F(TreePLRUDeathTest, TouchNull)
{
    ASSERT_DEATH(rp->touch(nullptr), "");
}

TEST_F(TreePLRUDeathTest, NoCandidates)
{
    gem5::ReplacementCandidates candidates;
    ASSERT_DEATH(rp->getVictim(candidates), "");
}

TEST_F(TreePLRUDeathTest, InvalidNumLeaves)
{
    gem5::TreePLRURPParams params;
    params.eventq_index = 0;
    params.num_leaves = 0;
    ASSERT_ANY_THROW(
        std::make_shared<gem5::replacement_policy::TreePLRU>(params));
}

/// We expect any operations on specific entries to fail for a TreePLRU
/// replacement policy that only accommodates one leaf.
TEST_F(OneLeafTreePLRUDeathTestF, OneLeafTree)
{
    ASSERT_ANY_THROW(rp->reset(entries[0].replacementData));
    ASSERT_ANY_THROW(rp->invalidate(entries[0].replacementData));
    ASSERT_ANY_THROW(rp->touch(entries[0].replacementData));
}
