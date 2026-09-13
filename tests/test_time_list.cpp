/**
 * @file test_time_list.cpp
 * @brief Unit tests for the tools::time_list class template.
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

#include <chrono>
#include <string>

#include "tools/time_list.hpp"

TEST(TimeListTest, TopIsEarliestTimestamp)
{
    tools::time_list<int, std::string> list;
    list.push(30, "third");
    list.push(10, "first");
    list.push(20, "second");

    EXPECT_EQ(list.size(), 3U);
    EXPECT_FALSE(list.empty());

    ASSERT_TRUE(list.top().has_value());
    EXPECT_EQ(list.top()->first, 10);
    EXPECT_EQ(list.top()->second, "first");
}

TEST(TimeListTest, TopPopDrainsInOrder)
{
    tools::time_list<int, std::string> list;
    list.push(30, "third");
    list.push(10, "first");
    list.push(20, "second");

    EXPECT_EQ(list.top_pop()->second, "first");
    EXPECT_EQ(list.top_pop()->second, "second");
    EXPECT_EQ(list.top_pop()->second, "third");
    EXPECT_FALSE(list.top_pop().has_value());
}

TEST(TimeListTest, Emplace)
{
    tools::time_list<int, std::string> list;
    list.emplace(5, "emplaced");

    EXPECT_EQ(list.top_pop()->second, "emplaced");
}

TEST(TimeListTest, PopRemovesEarliest)
{
    tools::time_list<int, int> list;
    list.push(2, 200);
    list.push(1, 100);
    list.pop();

    EXPECT_EQ(list.top()->first, 2);
    EXPECT_EQ(list.size(), 1U);
}

TEST(TimeListTest, ClearEmptiesList)
{
    tools::time_list<int, int> list;
    list.push(1, 100);
    list.push(2, 200);
    list.clear();

    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.size(), 0U);
}

TEST(TimeListTest, EmptyListHasNoTop)
{
    tools::time_list<int, int> list;
    EXPECT_FALSE(list.top().has_value());
    EXPECT_FALSE(list.top_pop().has_value());
}

TEST(TimeListTest, SupportsChronoTimePoint)
{
    using clock_type = std::chrono::steady_clock;
    tools::time_list<clock_type::time_point, int> list;

    const auto now = clock_type::now();
    list.push(now + std::chrono::milliseconds(20), 2);
    list.push(now + std::chrono::milliseconds(10), 1);

    EXPECT_EQ(list.top_pop()->second, 1);
    EXPECT_EQ(list.top_pop()->second, 2);
}
