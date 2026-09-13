/**
 * @file test_ring_buffer.cpp
 * @brief Unit tests for the tools::ring_buffer class template.
 *
 * @author Laurent Lardinois and Copilot
 * @date September 2026
 */
//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025-2026 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois //
//                                                                             //
// https://github.com/type-one/PublishSubscribe                                //
//                                                                             //
// MIT License                                                                 //
//                                                                             //
// This software is provided 'as-is', without any express or implied           //
// warranty.In no event will the authors be held liable for any damages        //
// arising from the use of this software.                                      //
//                                                                             //
// Permission is granted to anyone to use this software for any purpose,       //
// including commercial applications, and to alter itand redistribute it       //
// freely, subject to the following restrictions :                             //
//                                                                             //
// 1. The origin of this software must not be misrepresented; you must not     //
// claim that you wrote the original software.If you use this software         //
// in a product, an acknowledgment in the product documentation would be       //
// appreciated but is not required.                                            //
// 2. Altered source versions must be plainly marked as such, and must not be  //
// misrepresented as being the original software.                              //
// 3. This notice may not be removed or altered from any source distribution.  //
//-----------------------------------------------------------------------------//

#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "tools/ring_buffer.hpp"

TEST(RingBufferTest, PushPopFifoOrder)
{
    tools::ring_buffer<int, 4> buffer;
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));
    EXPECT_TRUE(buffer.push(4));
    EXPECT_FALSE(buffer.push(5)); // full

    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.back(), 4);
    EXPECT_EQ(buffer.size(), 4U);
    EXPECT_TRUE(buffer.full());

    buffer.pop();
    EXPECT_EQ(buffer.front(), 2);
    EXPECT_EQ(buffer.size(), 3U);
}

TEST(RingBufferTest, PushOverwriteEvictsOldest)
{
    tools::ring_buffer<int, 2> buffer;
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));

    EXPECT_TRUE(buffer.push_overwrite(3)); // overwrites 1
    EXPECT_EQ(buffer.front(), 2);
    EXPECT_EQ(buffer.back(), 3);
    EXPECT_EQ(buffer.size(), 2U);
}

TEST(RingBufferTest, Emplace)
{
    tools::ring_buffer<std::pair<int, int>, 2> buffer;
    EXPECT_TRUE(buffer.emplace(1, 2));
    EXPECT_EQ(buffer.front(), (std::pair<int, int> { 1, 2 }));
}

TEST(RingBufferTest, EmptyAndCapacity)
{
    tools::ring_buffer<int, 3> buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.capacity(), 3U);

    buffer.push(1);
    EXPECT_FALSE(buffer.empty());
}

TEST(RingBufferTest, PushRangeStopsWhenFull)
{
    tools::ring_buffer<int, 3> buffer;
    const std::vector<int> values = { 1, 2, 3, 4, 5 };
    const auto inserted = buffer.push_range(values.begin(), values.end());

    EXPECT_EQ(inserted, 3U);
    EXPECT_TRUE(buffer.full());
}

TEST(RingBufferTest, PopRangeReturnsEffectiveCount)
{
    tools::ring_buffer<int, 4> buffer;
    const std::vector<int> values = { 10, 20, 30 };
    buffer.push_range(values.begin(), values.end());

    std::array<int, 5> destination = { 0, 0, 0, 0, 0 };
    const auto popped = buffer.pop_range(destination.begin(), destination.end());

    EXPECT_EQ(popped, 3U);
    EXPECT_EQ(destination[0], 10);
    EXPECT_EQ(destination[1], 20);
    EXPECT_EQ(destination[2], 30);
    EXPECT_TRUE(buffer.empty());
}

TEST(RingBufferTest, ClearResetsState)
{
    tools::ring_buffer<int, 2> buffer;
    buffer.push(1);
    buffer.push(2);
    buffer.clear();

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_TRUE(buffer.push(10));
}

TEST(RingBufferTest, CopyAndMoveSemantics)
{
    tools::ring_buffer<int, 2> buffer;
    buffer.push(1);
    buffer.push(2);

    tools::ring_buffer<int, 2> copy(buffer);
    EXPECT_EQ(copy.front(), 1);
    EXPECT_EQ(copy.size(), 2U);

    tools::ring_buffer<int, 2> moved(std::move(copy));
    EXPECT_EQ(moved.front(), 1);
    EXPECT_EQ(moved.size(), 2U);
}
