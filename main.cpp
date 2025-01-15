//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois      //
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

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <type_traits>

#include "tools/async_observer.hpp"
#include "tools/histogram.hpp"
#include "tools/periodic_task.hpp"
#include "tools/sync_dictionary.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_queue.hpp"

void test_sync_queue()
{
    tools::sync_queue<std::string> str_queue;

    str_queue.push("toto");

    auto item = str_queue.front();

    str_queue.pop();
}

void test_sync_dictionary()
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    str_dict.add("toto", "blob");

    auto result = str_dict.find("toto");

    if (result.has_value())
    {
        str_dict.remove("toto");
    }
}


enum class my_topic
{
    generic,
    system,
    external
};

using base_observer = tools::sync_observer<my_topic, std::string>;
class my_observer : public base_observer
{
public:
    my_observer() = default;
    virtual ~my_observer() { }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "sync [topic " << static_cast<std::underlying_type<my_topic>::type>(topic) << "] received: event (" << event
                  << ") from " << origin << std::endl;
    }

private:
};

using base_async_observer = tools::async_observer<my_topic, std::string>;
class my_async_observer : public base_async_observer
{
public:
    my_async_observer()
        : m_task_loop([this]() { handle_events(); })
    {
    }

    virtual ~my_async_observer()
    {
        m_stop_task.store(true);
        m_task_loop.join();
    }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "async/push [topic " << static_cast<std::underlying_type<my_topic>::type>(topic) << "] received: event (" << event
                  << ") from " << origin << std::endl;

        base_async_observer::inform(topic, event, origin);
    }

private:
    void handle_events()
    {
        const auto timeout = std::chrono::duration<int, std::micro>(1000);

        while (!m_stop_task.load())
        {
            wait_for_events(timeout);

            while (number_of_events() > 0)
            {
                auto entry = pop_first_event();
                if (entry.has_value())
                {
                    auto& [topic, event, origin] = *entry;

                    std::cout << "async/pop [topic " << static_cast<std::underlying_type<my_topic>::type>(topic) << "] received: event ("
                              << event << ") from " << origin << std::endl;
                }
            }
        }
    }

    std::thread m_task_loop;
    std::atomic_bool m_stop_task = false;
};

using base_subject = tools::sync_subject<my_topic, std::string>;
class my_subject : public base_subject
{
public:
    my_subject() = delete;
    my_subject(const std::string name)
        : base_subject(name)
    {
    }

    virtual ~my_subject() { }

    virtual void publish(const my_topic& topic, const std::string& event) override
    {
        std::cout << "publish: event (" << event << ") to " << name() << std::endl;
        base_subject::publish(topic, event);
    }

private:
};

void test_publish_subscribe()
{
    auto observer1 = std::make_shared<my_observer>();
    auto observer2 = std::make_shared<my_observer>();
    auto async_observer = std::make_shared<my_async_observer>();
    auto subject1 = std::make_shared<my_subject>("source1");
    auto subject2 = std::make_shared<my_subject>("source2");

    subject1->subscribe(my_topic::generic, observer1);
    subject1->subscribe(my_topic::generic, observer2);
    subject1->subscribe(my_topic::system, observer2);
    subject1->subscribe(my_topic::generic, async_observer);

    subject2->subscribe(my_topic::generic, observer1);
    subject2->subscribe(my_topic::generic, observer2);
    subject2->subscribe(my_topic::system, observer2);
    subject2->subscribe(my_topic::generic, async_observer);

    subject1->publish(my_topic::generic, "toto");

    subject1->unsubscribe(my_topic::generic, observer1);

    subject1->publish(my_topic::generic, "titi");

    subject1->publish(my_topic::system, "tata");

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(500));

    subject1->publish(my_topic::generic, "tintin");

    subject2->publish(my_topic::generic, "tonton");
    subject2->publish(my_topic::system, "tantine");
}

struct my_periodic_task_context
{
    std::atomic<int> loop_counter = 0;
    tools::sync_queue<std::chrono::high_resolution_clock::time_point> time_points;
};

using my_periodic_task = tools::periodic_task<my_periodic_task_context>;

void test_periodic_task()
{
    auto lambda = [](std::shared_ptr<my_periodic_task_context> context) -> void
    {
        context->loop_counter += 1;
        context->time_points.emplace(std::chrono::high_resolution_clock::now());
    };

    auto context = std::make_shared<my_periodic_task_context>();
    // 10 ms period
    const auto period = std::chrono::duration<int, std::micro>(10000);
    const auto start_timepoint = std::chrono::high_resolution_clock::now();
    my_periodic_task task1(lambda, context, period);

    // sleep 2 sec
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));

    std::cout << "nb of periodic loops = " << context->loop_counter << std::endl;

    auto previous_timepoint = start_timepoint;
    while (!context->time_points.empty())
    {
        const auto measured_timepoint = context->time_points.front();
        context->time_points.pop();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(measured_timepoint - previous_timepoint);
        std::cout << "timepoint: " << elapsed.count() << " us" << std::endl;
        previous_timepoint = measured_timepoint;
    }
}

class my_collector : public base_observer
{
public:
    my_collector() = default;
    virtual ~my_collector() { }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        (void)topic;
        (void)origin;

        m_histogram.add(static_cast<double>(std::strtod(event.c_str(), nullptr)));
    }

    void display_stats()
    {
        auto top = m_histogram.top();
        std::cout << std::endl << "value " << top << " appears " << m_histogram.top_occurence() << " times" << std::endl;
        auto avg = m_histogram.average();
        std::cout << "average value is " << avg << std::endl;
        std::cout << "median value is " << m_histogram.median() << std::endl;
        auto variance = m_histogram.variance(avg);
        std::cout << "variance is " << variance << std::endl;
        std::cout << "gaussian probability of " << top << " occuring is " << m_histogram.gaussian_probability(top, avg, variance) << std::endl;
    }

private:
    tools::histogram<double> m_histogram;
};

void test_periodic_publish_subscribe()
{
    auto monitoring = std::make_shared<my_async_observer>();
    auto data_source = std::make_shared<my_subject>("data_source");
    auto histogram_feeder = std::make_shared<my_collector>();

    auto sampler = [&data_source](std::shared_ptr<my_periodic_task_context> context) -> void
    {
        context->loop_counter += 1;

        // mocked signal
        double signal = std::sin(context->loop_counter.load());

        // emit "signal" as a 'string' event
        data_source->publish(my_topic::external, std::to_string(signal));
    };

    data_source->subscribe(my_topic::external, monitoring);
    data_source->subscribe(my_topic::external, histogram_feeder);

    // "sample" with a 100 ms period
    auto context = std::make_shared<my_periodic_task_context>();
    const auto period = std::chrono::duration<int, std::milli>(100);
    {
        my_periodic_task periodic_task(sampler, context, period);

        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));
    }

    histogram_feeder->display_stats();
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    test_sync_queue();
    test_sync_dictionary();
    test_publish_subscribe();
    test_periodic_task();
    test_periodic_publish_subscribe();

    return 0;
}
