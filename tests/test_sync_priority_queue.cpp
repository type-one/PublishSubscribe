/**
 * @file test_sync_priority_queue.cpp
 * @brief Unit tests for the tools::sync_priority_queue class template.
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

#include <string>
#include <vector>

#include "tools/sync_priority_queue.hpp"

TEST(SyncPriorityQueueTest, DefaultIsMinHeap)
{
    tools::sync_priority_queue<int> queue;
    queue.push(5);
    queue.push(1);
    queue.push(3);

    EXPECT_EQ(queue.top_pop().value(), 1);
    EXPECT_EQ(queue.top_pop().value(), 3);
    EXPECT_EQ(queue.top_pop().value(), 5);
    EXPECT_FALSE(queue.top_pop().has_value());
}

TEST(SyncPriorityQueueTest, MaxPriorityQueueAlias)
{
    tools::sync_max_priority_queue<int> queue;
    queue.push(5);
    queue.push(1);
    queue.push(3);

    EXPECT_EQ(queue.top_pop().value(), 5);
    EXPECT_EQ(queue.top_pop().value(), 3);
    EXPECT_EQ(queue.top_pop().value(), 1);
}

TEST(SyncPriorityQueueTest, TopDoesNotConsume)
{
    tools::sync_priority_queue<int> queue;
    queue.push(10);
    queue.push(2);

    EXPECT_EQ(queue.top().value(), 2);
    EXPECT_EQ(queue.size(), 2U);
    queue.pop();
    EXPECT_EQ(queue.top().value(), 10);
}

TEST(SyncPriorityQueueTest, Emplace)
{
    tools::sync_priority_queue<std::string> queue;
    queue.emplace("banana");
    queue.emplace("apple");

    EXPECT_EQ(queue.top_pop().value(), "apple");
    EXPECT_EQ(queue.top_pop().value(), "banana");
}

TEST(SyncPriorityQueueTest, EmptyAndSize)
{
    tools::sync_priority_queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0U);

    queue.push(1);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1U);
}

TEST(SyncPriorityQueueTest, PushRange)
{
    tools::sync_priority_queue<int> queue;
    const std::vector<int> values = { 4, 2, 8, 1 };
    queue.push_range(values.begin(), values.end());

    EXPECT_EQ(queue.top_pop().value(), 1);
    EXPECT_EQ(queue.top_pop().value(), 2);
    EXPECT_EQ(queue.top_pop().value(), 4);
    EXPECT_EQ(queue.top_pop().value(), 8);
}

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
TEST(SyncPriorityQueueTest, PushRangeFromRange)
{
    tools::sync_priority_queue<int> queue;
    const std::vector<int> values = { 9, 3, 6 };
    queue.push_range(values);

    EXPECT_EQ(queue.top_pop().value(), 3);
    EXPECT_EQ(queue.top_pop().value(), 6);
    EXPECT_EQ(queue.top_pop().value(), 9);
}
#endif

TEST(SyncPriorityQueueTest, FrontBackAliasesMapToTop)
{
    tools::sync_priority_queue<int> queue;
    queue.push(7);
    queue.push(3);

    EXPECT_EQ(queue.front().value(), 3);
    EXPECT_EQ(queue.back().value(), 3);
    EXPECT_EQ(queue.front_pop().value(), 3);
}
