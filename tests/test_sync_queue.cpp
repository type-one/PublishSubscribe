/**
 * @file test_sync_queue.cpp
 * @brief Unit tests for the tools::sync_queue class template.
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

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <array>
#include <span>
#endif

#include "tools/sync_queue.hpp"

TEST(SyncQueueTest, PushAndFrontPop)
{
    tools::sync_queue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(queue.size(), 3U);
    EXPECT_FALSE(queue.empty());

    EXPECT_EQ(queue.front_pop().value(), 1);
    EXPECT_EQ(queue.front_pop().value(), 2);
    EXPECT_EQ(queue.front_pop().value(), 3);
    EXPECT_FALSE(queue.front_pop().has_value());
}

TEST(SyncQueueTest, FrontAndBackDoNotConsume)
{
    tools::sync_queue<std::string> queue;
    queue.push("first");
    queue.push("second");

    EXPECT_EQ(queue.front().value(), "first");
    EXPECT_EQ(queue.back().value(), "second");
    EXPECT_EQ(queue.size(), 2U);
}

TEST(SyncQueueTest, Emplace)
{
    tools::sync_queue<std::string> queue;
    queue.emplace("emplaced");

    EXPECT_EQ(queue.front_pop().value(), "emplaced");
}

TEST(SyncQueueTest, PopDropsFrontElement)
{
    tools::sync_queue<int> queue;
    queue.push(1);
    queue.push(2);
    queue.pop();

    EXPECT_EQ(queue.front().value(), 2);
    EXPECT_EQ(queue.size(), 1U);
}

TEST(SyncQueueTest, PushRangeAndPopRange)
{
    tools::sync_queue<int> queue;
    const std::vector<int> values = { 1, 2, 3, 4 };
    queue.push_range(values.begin(), values.end());

    EXPECT_EQ(queue.size(), 4U);

    std::vector<int> destination(3, 0);
    const auto popped = queue.pop_range(destination.begin(), destination.end());

    EXPECT_EQ(popped, 3U);
    EXPECT_EQ(destination[0], 1);
    EXPECT_EQ(destination[1], 2);
    EXPECT_EQ(destination[2], 3);
    EXPECT_EQ(queue.size(), 1U);
}

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
TEST(SyncQueueTest, PushRangeFromRangeAndSpanPop)
{
    tools::sync_queue<int> queue;
    const std::vector<int> values = { 10, 20, 30 };
    queue.push_range(values);

    std::array<int, 2> destination = { 0, 0 };
    const auto popped = queue.pop_range(std::span<int>(destination));

    EXPECT_EQ(popped, 2U);
    EXPECT_EQ(destination[0], 10);
    EXPECT_EQ(destination[1], 20);
    EXPECT_EQ(queue.front().value(), 30);
}
#endif

TEST(SyncQueueTest, ConcurrentPushAndPop)
{
    tools::sync_queue<int> queue;
    std::atomic<int> pushed_count { 0 };

    std::thread producer(
        [&queue, &pushed_count]()
        {
            for (int i = 0; i < 1000; ++i)
            {
                queue.push(i);
                pushed_count.fetch_add(1);
            }
        });

    std::thread consumer(
        [&queue, &pushed_count]()
        {
            int consumed = 0;
            while (consumed < 1000)
            {
                auto item = queue.front_pop();
                if (item.has_value())
                {
                    ++consumed;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        });

    producer.join();
    consumer.join();

    EXPECT_EQ(pushed_count.load(), 1000);
    EXPECT_TRUE(queue.empty());
}
