/**
 * @file test_sync_ring_buffer.cpp
 * @brief Unit tests for the tools::sync_ring_buffer class template.
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
#include <thread>
#include <vector>

#include "tools/sync_ring_buffer.hpp"

TEST(SyncRingBufferTest, PushPopFifoOrder)
{
    tools::sync_ring_buffer<int, 4> buffer;
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

TEST(SyncRingBufferTest, PushOverwriteEvictsOldest)
{
    tools::sync_ring_buffer<int, 2> buffer;
    buffer.push(1);
    buffer.push(2);
    EXPECT_TRUE(buffer.push_overwrite(3));

    std::array<int, 2> destination {};
    buffer.pop_range(destination.begin(), destination.end());
    EXPECT_EQ(destination[0], 2);
    EXPECT_EQ(destination[1], 3);
}

TEST(SyncRingBufferTest, Emplace)
{
    tools::sync_ring_buffer<std::pair<int, int>, 2> buffer;
    EXPECT_TRUE(buffer.emplace(1, 2));
}

TEST(SyncRingBufferTest, PushRangeStopsWhenFull)
{
    tools::sync_ring_buffer<int, 3> buffer;
    const std::vector<int> values = { 1, 2, 3, 4, 5 };
    const auto inserted = buffer.push_range(values.begin(), values.end());

    EXPECT_EQ(inserted, 3U);
}

TEST(SyncRingBufferTest, ConcurrentPushAndPop)
{
    tools::sync_ring_buffer<int, 16> buffer;
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
