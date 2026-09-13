/**
 * @file test_sync_time_list.cpp
 * @brief Unit tests for the tools::sync_time_list class template.
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

#include "tools/sync_time_list.hpp"

TEST(SyncTimeListTest, TopIsEarliestTimestamp)
{
    tools::sync_time_list<int, std::string> list;
    list.push(30, "third");
    list.push(10, "first");
    list.push(20, "second");

    EXPECT_EQ(list.size(), 3U);
    EXPECT_EQ(list.top()->second, "first");
}

TEST(SyncTimeListTest, TopPopDrainsInOrder)
{
    tools::sync_time_list<int, std::string> list;
    list.push(30, "third");
    list.push(10, "first");
    list.push(20, "second");

    EXPECT_EQ(list.top_pop()->second, "first");
    EXPECT_EQ(list.top_pop()->second, "second");
    EXPECT_EQ(list.top_pop()->second, "third");
    EXPECT_FALSE(list.top_pop().has_value());
}

TEST(SyncTimeListTest, Emplace)
{
    tools::sync_time_list<int, std::string> list;
    list.emplace(5, "emplaced");

    EXPECT_EQ(list.top_pop()->second, "emplaced");
}

TEST(SyncTimeListTest, EmptyAndSize)
{
    tools::sync_time_list<int, int> list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0U);

    list.push(1, 100);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(list.size(), 1U);
}

TEST(SyncTimeListTest, ConcurrentPushAndTopPop)
{
    tools::sync_time_list<int, int> list;
    std::atomic<int> pushed_count { 0 };

    std::thread producer(
        [&list, &pushed_count]()
        {
            for (int i = 0; i < 500; ++i)
            {
                list.push(i, i);
                pushed_count.fetch_add(1);
            }
        });

    std::thread consumer(
        [&list, &pushed_count]()
        {
            int consumed = 0;
            while (consumed < 500)
            {
                auto item = list.top_pop();
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

    EXPECT_EQ(pushed_count.load(), 500);
    EXPECT_TRUE(list.empty());
}
