/**
 * @file test_ring_vector.cpp
 * @brief Unit tests for the tools::ring_vector class template.
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

#include <vector>

#include "tools/ring_vector.hpp"

TEST(RingVectorTest, PushPopFifoOrder)
{
    tools::ring_vector<int> buffer(4);
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));
    EXPECT_TRUE(buffer.push(4));
    EXPECT_FALSE(buffer.push(5)); // full

    EXPECT_EQ(buffer.front(), 1);
    EXPECT_EQ(buffer.back(), 4);
    EXPECT_TRUE(buffer.full());

    buffer.pop();
    EXPECT_EQ(buffer.front(), 2);
}

TEST(RingVectorTest, PushOverwriteEvictsOldest)
{
    tools::ring_vector<int> buffer(2);
    buffer.push(1);
    buffer.push(2);

    EXPECT_TRUE(buffer.push_overwrite(3));
    EXPECT_EQ(buffer.front(), 2);
    EXPECT_EQ(buffer.back(), 3);
}

TEST(RingVectorTest, Emplace)
{
    tools::ring_vector<std::pair<int, int>> buffer(2);
    EXPECT_TRUE(buffer.emplace(1, 2));
    EXPECT_EQ(buffer.front(), (std::pair<int, int> { 1, 2 }));
}

TEST(RingVectorTest, PushRangeStopsWhenFull)
{
    tools::ring_vector<int> buffer(3);
    const std::vector<int> values = { 1, 2, 3, 4, 5 };
    const auto inserted = buffer.push_range(values.begin(), values.end());

    EXPECT_EQ(inserted, 3U);
    EXPECT_TRUE(buffer.full());
}

TEST(RingVectorTest, ClearResetsState)
{
    tools::ring_vector<int> buffer(2);
    buffer.push(1);
    buffer.push(2);
    buffer.clear();

    EXPECT_TRUE(buffer.empty());
    EXPECT_TRUE(buffer.push(10));
}

TEST(RingVectorTest, CopyAndMoveSemantics)
{
    tools::ring_vector<int> buffer(2);
    buffer.push(1);
    buffer.push(2);

    tools::ring_vector<int> copy(buffer);
    EXPECT_EQ(copy.front(), 1);

    tools::ring_vector<int> moved(std::move(copy));
    EXPECT_EQ(moved.front(), 1);
}
