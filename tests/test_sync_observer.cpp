/**
 * @file test_sync_observer.cpp
 * @brief Unit tests for the tools::sync_observer and tools::sync_subject classes.
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

#include <memory>
#include <string>
#include <vector>

#include "tools/sync_observer.hpp"

namespace
{
    class recording_observer : public tools::sync_observer<std::string, std::string>
    {
    public:
        void inform(const std::string& topic, const std::string& event, const std::string& origin) override
        {
            m_received.emplace_back(topic, event, origin);
        }

        std::vector<std::tuple<std::string, std::string, std::string>> m_received;
    };
}

TEST(SyncObserverTest, SubscribeAndPublishInformsObserver)
{
    tools::sync_subject<std::string, std::string> subject("TestSubject");
    auto observer = std::make_shared<recording_observer>();

    subject.subscribe("topic_1", observer);
    subject.publish("topic_1", "event1");

    ASSERT_EQ(observer->m_received.size(), 1U);
    EXPECT_EQ(std::get<0>(observer->m_received[0]), "topic_1");
    EXPECT_EQ(std::get<1>(observer->m_received[0]), "event1");
    EXPECT_EQ(std::get<2>(observer->m_received[0]), "TestSubject");
}

TEST(SyncObserverTest, UnsubscribeStopsFurtherNotifications)
{
    tools::sync_subject<std::string, std::string> subject("TestSubject");
    auto observer = std::make_shared<recording_observer>();

    subject.subscribe("topic_1", observer);
    subject.unsubscribe("topic_1", observer);
    subject.publish("topic_1", "event1");

    EXPECT_TRUE(observer->m_received.empty());
}

TEST(SyncObserverTest, MultipleObserversReceiveSameEvent)
{
    tools::sync_subject<std::string, std::string> subject("TestSubject");
    auto observer1 = std::make_shared<recording_observer>();
    auto observer2 = std::make_shared<recording_observer>();

    subject.subscribe("topic_1", observer1);
    subject.subscribe("topic_1", observer2);
    subject.publish("topic_1", "event1");

    ASSERT_EQ(observer1->m_received.size(), 1U);
    ASSERT_EQ(observer2->m_received.size(), 1U);
}

TEST(SyncObserverTest, HandlerSubscriptionReceivesEvent)
{
    tools::sync_subject<std::string, std::string> subject("TestSubject");

    std::vector<std::tuple<std::string, std::string, std::string>> received;
    subject.subscribe("topic_1", "handler_1",
        [&received](const std::string& topic, const std::string& event, const std::string& origin)
        { received.emplace_back(topic, event, origin); });

    subject.publish("topic_1", "event1");

    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(std::get<1>(received[0]), "event1");
}

TEST(SyncObserverTest, UnsubscribeHandlerByName)
{
    tools::sync_subject<std::string, std::string> subject("TestSubject");

    std::vector<std::string> received;
    subject.subscribe("topic_1", "handler_1",
        [&received](const std::string&, const std::string& event, const std::string&) { received.push_back(event); });

    subject.unsubscribe("topic_1", "handler_1");
    subject.publish("topic_1", "event1");

    EXPECT_TRUE(received.empty());
}

TEST(SyncObserverTest, NameReturnsSubjectName)
{
    tools::sync_subject<std::string, std::string> subject("MySubject");
    EXPECT_EQ(subject.name(), "MySubject");
}
