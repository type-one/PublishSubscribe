/**
 * @file test_async_observer.cpp
 * @brief Unit tests for the tools::async_observer class template.
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
#include <memory>
#include <string>
#include <thread>

#include "tools/async_observer.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_queue.hpp"
#include "tools/sync_ring_buffer.hpp"
#include "tools/sync_ring_vector.hpp"

class AsyncObserverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        subject = std::make_unique<tools::sync_subject<std::string, std::string>>("TestSubject");
    }

    void TearDown() override
    {
        subject.reset();
    }

    std::unique_ptr<tools::sync_subject<std::string, std::string>> subject;
};

TEST_F(AsyncObserverTest, SingleObserverSingleEvent)
{
    auto observer = std::make_shared<tools::async_observer<std::string, std::string, tools::sync_queue>>();
    subject->subscribe("topic_1", observer);

    subject->publish("topic_1", "event1");
    observer->wait_for_events();

    auto events = observer->pop_all_events();
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<0>(events[0]), "topic_1");
    EXPECT_EQ(std::get<1>(events[0]), "event1");
    EXPECT_EQ(std::get<2>(events[0]), "TestSubject");
}

TEST_F(AsyncObserverTest, SingleObserverMultipleEvents)
{
    auto observer = std::make_shared<tools::async_observer<std::string, std::string, tools::sync_queue>>();
    subject->subscribe("topic_1", observer);
    subject->subscribe("topic_2", observer);

    subject->publish("topic_1", "event1");
    subject->publish("topic_2", "event2");
    observer->wait_for_events();

    auto events = observer->pop_all_events();
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(std::get<1>(events[0]), "event1");
    EXPECT_EQ(std::get<1>(events[1]), "event2");
}

TEST_F(AsyncObserverTest, PopFirstAndLastEvent)
{
    auto observer = std::make_shared<tools::async_observer<std::string, std::string, tools::sync_queue>>();
    subject->subscribe("topic_1", observer);

    subject->publish("topic_1", "event1");
    subject->publish("topic_1", "event2");
    subject->publish("topic_1", "event3");
    observer->wait_for_events();

    auto first = observer->pop_first_event();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(std::get<1>(*first), "event1");

    auto last = observer->pop_last_event();
    ASSERT_TRUE(last.has_value());
    EXPECT_EQ(std::get<1>(*last), "event3");

    EXPECT_FALSE(observer->has_events());
}

TEST_F(AsyncObserverTest, WaitForEventsWithTimeoutExpires)
{
    auto observer = std::make_shared<tools::async_observer<std::string, std::string, tools::sync_queue>>();
    subject->subscribe("topic_1", observer);

    observer->wait_for_events(std::chrono::milliseconds(50));
    EXPECT_FALSE(observer->has_events());
}

TEST(AsyncObserverQueueOverflowTest, BoundedContainerReportsDroppedEntryWhenFull)
{
    static constexpr std::size_t queue_depth = 2U;
    tools::async_observer<std::string, std::string, tools::sync_ring_vector> observer(queue_depth);

    observer.inform("topic", "event-1", "producer");
    EXPECT_FALSE(observer.has_queue_overflow());

    observer.inform("topic", "event-2", "producer");
    EXPECT_FALSE(observer.has_queue_overflow());

    observer.inform("topic", "event-3-dropped", "producer");
    EXPECT_TRUE(observer.has_queue_overflow());
    EXPECT_EQ(observer.queue_overflow_count(), 1U);

    auto events = observer.pop_all_events();
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(std::get<1>(events[0]), "event-1");
    EXPECT_EQ(std::get<1>(events[1]), "event-2");
}

TEST(AsyncObserverQueueOverflowTest, ConsumeQueueOverflowCountResetsCounter)
{
    static constexpr std::size_t queue_depth = 1U;
    tools::async_observer<std::string, std::string, tools::sync_ring_vector> observer(queue_depth);

    observer.inform("topic", "event-1", "producer");
    observer.inform("topic", "event-2-dropped", "producer");
    observer.inform("topic", "event-3-dropped", "producer");

    EXPECT_EQ(observer.consume_queue_overflow_count(), 2U);
    EXPECT_FALSE(observer.has_queue_overflow());
    EXPECT_EQ(observer.consume_queue_overflow_count(), 0U);

    // draining the queue frees capacity; a subsequent drop is counted again from zero
    auto drained = observer.pop_all_events();
    ASSERT_EQ(drained.size(), 1U);

    observer.inform("topic", "event-4", "producer");
    observer.inform("topic", "event-5-dropped", "producer");
    EXPECT_EQ(observer.consume_queue_overflow_count(), 1U);
}

TEST(AsyncObserverQueueOverflowTest, UnboundedContainerNeverReportsOverflow)
{
    tools::async_observer<std::string, std::string, tools::sync_queue> observer;

    static constexpr int informed_event_count = 100;
    for (int index = 0; index < informed_event_count; ++index)
    {
        observer.inform("topic", "event", "producer");
    }

    EXPECT_FALSE(observer.has_queue_overflow());
    EXPECT_EQ(observer.queue_overflow_count(), 0U);
    EXPECT_EQ(observer.number_of_events(), static_cast<std::size_t>(informed_event_count));
}

namespace
{
    template <typename T>
    using fixed_capacity_ring_buffer_2 = tools::sync_ring_buffer<T, 2U>;
}

TEST(AsyncObserverQueueOverflowTest, FixedCapacityRingBufferReportsDroppedEntryWhenFull)
{
    tools::async_observer<std::string, std::string, fixed_capacity_ring_buffer_2> observer;

    observer.inform("topic", "event-1", "producer");
    EXPECT_FALSE(observer.has_queue_overflow());

    observer.inform("topic", "event-2", "producer");
    EXPECT_FALSE(observer.has_queue_overflow());

    observer.inform("topic", "event-3-dropped", "producer");
    EXPECT_TRUE(observer.has_queue_overflow());
    EXPECT_EQ(observer.queue_overflow_count(), 1U);

    auto events = observer.pop_all_events();
    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(std::get<1>(events[0]), "event-1");
    EXPECT_EQ(std::get<1>(events[1]), "event-2");
}

TEST(AsyncObserverPerfectForwardingTest, InformSupportsLvalueRvalueAndConversion)
{
    tools::async_observer<std::string, std::string, tools::sync_queue> observer;

    std::string topic_lvalue = "topic-lvalue";
    std::string event_lvalue = "event-lvalue";
    std::string origin_lvalue = "origin-lvalue";

    observer.inform(topic_lvalue, event_lvalue, origin_lvalue);
    observer.inform(std::string("topic-rvalue"), std::string("event-rvalue"), std::string("origin-rvalue"));
    observer.inform("topic-conversion", "event-conversion", "origin-conversion");

    auto events = observer.pop_all_events();
    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(std::get<0>(events[0]), "topic-lvalue");
    EXPECT_EQ(std::get<0>(events[1]), "topic-rvalue");
    EXPECT_EQ(std::get<0>(events[2]), "topic-conversion");
}
