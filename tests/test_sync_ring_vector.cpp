/**
 * @file test_sync_ring_vector.cpp
 * @brief Unit tests for the tools::sync_ring_vector class template.
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
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

#include "tools/sync_ring_vector.hpp"

TEST(SyncRingVectorTest, PushPopFifoOrder)
{
    tools::sync_ring_vector<int> buffer(4);
    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.push(3));
    EXPECT_TRUE(buffer.push(4));
    EXPECT_FALSE(buffer.push(5));

    std::array<int, 4> destination {};
    const auto popped = buffer.pop_range(destination.begin(), destination.end());
    EXPECT_EQ(popped, 4U);
    EXPECT_EQ(destination[0], 1);
    EXPECT_EQ(destination[3], 4);
}

TEST(SyncRingVectorTest, PushOverwriteEvictsOldest)
{
    tools::sync_ring_vector<int> buffer(2);
    buffer.push(1);
    buffer.push(2);
    EXPECT_TRUE(buffer.push_overwrite(3));

    std::array<int, 2> destination {};
    buffer.pop_range(destination.begin(), destination.end());
    EXPECT_EQ(destination[0], 2);
    EXPECT_EQ(destination[1], 3);
}

TEST(SyncRingVectorTest, Emplace)
{
    tools::sync_ring_vector<std::pair<int, int>> buffer(2);
    EXPECT_TRUE(buffer.emplace(1, 2));
}

TEST(SyncRingVectorTest, ConcurrentPushAndPop)
{
    tools::sync_ring_vector<int> buffer(16);
    std::atomic<int> produced { 0 };
    std::atomic<int> consumed { 0 };

    std::thread producer(
        [&buffer, &produced]()
        {
            for (int i = 0; i < 1000; ++i)
            {
                while (!buffer.push(i))
                {
                    std::this_thread::yield();
                }
                produced.fetch_add(1);
            }
        });

    std::thread consumer(
        [&buffer, &consumed]()
        {
            std::array<int, 1> destination {};
            while (consumed.load() < 1000)
            {
                if (buffer.pop_range(destination.begin(), destination.end()) == 1U)
                {
                    consumed.fetch_add(1);
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced.load(), 1000);
    EXPECT_EQ(consumed.load(), 1000);
}

TEST(SyncRingVectorTest, ResizeToOccupancyPreservesRecord)
{
    constexpr std::size_t initial_capacity = 1024U;
    constexpr std::size_t target_capacity = 1U;
    tools::sync_ring_vector<int> buffer(initial_capacity);
    ASSERT_TRUE(buffer.push(42));

    buffer.resize(target_capacity);

    EXPECT_EQ(buffer.capacity(), target_capacity);
    ASSERT_EQ(buffer.size(), target_capacity);
    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.push(99));
    std::vector<int> destination(target_capacity);
    ASSERT_EQ(buffer.pop_range(destination.begin(), destination.end()), target_capacity);
    EXPECT_EQ(destination.front(), 42);
    EXPECT_TRUE(buffer.empty());
    EXPECT_TRUE(buffer.push(99));
}

TEST(SyncRingVectorTest, ResizeEmptyRingToZeroAndRegrow)
{
    constexpr std::size_t initial_capacity = 1024U;
    tools::sync_ring_vector<int> buffer(initial_capacity);

    buffer.resize(0U);

    EXPECT_EQ(buffer.capacity(), 0U);
    EXPECT_TRUE(buffer.empty());
    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.push(42));

    buffer.resize(initial_capacity);

    EXPECT_EQ(buffer.capacity(), initial_capacity);
    EXPECT_TRUE(buffer.empty());
    ASSERT_TRUE(buffer.push(99));
    std::vector<int> destination(1U);
    ASSERT_EQ(buffer.pop_range(destination.begin(), destination.end()), 1U);
    EXPECT_EQ(destination.front(), 99);
}
