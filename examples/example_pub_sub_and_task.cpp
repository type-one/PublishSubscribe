/**
 * @file example_pub_sub_and_task.cpp
 * @brief Runs publish/subscribe and periodic task examples.
 *
 * @author Laurent Lardinois
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

#include "example_common.hpp"

enum class my_topic : std::uint8_t
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
    ~my_observer() override = default;

    void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "sync [topic " << static_cast<unsigned int>(topic) << "] received: event (" << event << ") from "
                  << origin << '\n';
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

    ~my_async_observer() override
    {
        m_stop_task.store(true);
        m_task_loop.join();
    }

    void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "async/push [topic " << static_cast<unsigned int>(topic) << "] received: event (" << event
                  << ") from " << origin << '\n';

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

                    std::cout << "async/pop [topic " << static_cast<unsigned int>(topic) << "] received: event ("
                              << event << ") from " << origin << '\n';
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
    my_subject(const std::string& name)
        : base_subject(name)
    {
    }

    ~my_subject() override = default;

    void publish(const my_topic& topic, const std::string& event) const override
    {
        std::cout << "publish: event (" << event << ") to " << name() << '\n';
        base_subject::publish(topic, event);
    }

private:
};

void test_publish_subscribe()
{
    std::cout << "-- publish subscribe --" << '\n';
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

    subject1->subscribe(my_topic::generic, "loose_coupled_handler_1",
        [](const my_topic& topic, const std::string& event, const std::string& origin)
        {
            std::cout << "handler [topic " << static_cast<unsigned int>(topic) << "] received: event (" << event
                      << ") from " << origin << '\n';
        });

    subject1->publish(my_topic::generic, "toto");

    subject1->unsubscribe(my_topic::generic, observer1);

    subject1->publish(my_topic::generic, "titi");

    subject1->publish(my_topic::system, "tata");

    subject1->unsubscribe(my_topic::generic, "loose_coupled_handler_1");

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(500));

    subject1->publish(my_topic::generic, "tintin");

    subject2->publish(my_topic::generic, "tonton");
    subject2->publish(my_topic::system, "tantine");
}

//--------------------------------------------------------------------------------------------------------------------------------

struct my_periodic_task_context
{
    std::atomic<int> loop_counter = 0;
    tools::sync_queue<std::chrono::high_resolution_clock::time_point> time_points;
};

using my_periodic_task = tools::periodic_task<my_periodic_task_context>;

void test_periodic_task()
{
    std::cout << "-- periodic task --" << '\n';
    auto startup = [](const std::shared_ptr<my_periodic_task_context>& context, const std::string& task_name) -> void
    {
        (void)task_name;
        context->loop_counter = 0;
    };

    auto lambda = [](const std::shared_ptr<my_periodic_task_context>& context, const std::string& task_name) -> void
    {
        (void)task_name;
        context->loop_counter += 1;
        context->time_points.emplace(std::chrono::high_resolution_clock::now());
    };

    auto context = std::make_shared<my_periodic_task_context>();
    // 20 ms period
    static constexpr auto period = std::chrono::duration<int, std::micro>(20000);
    const auto start_timepoint = std::chrono::high_resolution_clock::now();
    my_periodic_task task1(startup, lambda, context, "periodic task 1", period);

    // sleep 2 sec
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));

    std::cout << "nb of periodic loops = " << context->loop_counter.load() << '\n';

    auto previous_timepoint = start_timepoint;
    while (!context->time_points.empty())
    {
        const auto measured_timepoint = context->time_points.front_pop();

        if (measured_timepoint.has_value())
        {
            const auto elapsed
                = std::chrono::duration_cast<std::chrono::microseconds>(*measured_timepoint - previous_timepoint);
            std::cout << "timepoint: " << elapsed.count() << " us" << '\n';
            previous_timepoint = *measured_timepoint;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

class my_collector : public base_observer
{
public:
    my_collector() = default;
    ~my_collector() override = default;

    void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        (void)topic;
        (void)origin;

        m_histogram.add(std::strtod(event.c_str(), nullptr));
    }

    void display_stats()
    {
        const auto top = m_histogram.top();
        std::cout << '\n' << "value " << top << " appears " << m_histogram.top_occurence() << " times" << '\n';
        const auto avg = m_histogram.average();
        std::cout << "average value is " << avg << '\n';
        std::cout << "median value is " << m_histogram.median() << '\n';
        const auto variance = m_histogram.variance(avg);
        std::cout << "variance is " << variance << '\n';
        const auto std_deviation = m_histogram.standard_deviation(variance);
        std::cout << "standard deviation is " << std_deviation << '\n';
        std::cout << "gaussian probability of [" << std::floor(top) << "," << std::ceil(top) << "] occuring is "
                  << m_histogram.gaussian_probability(std::floor(top), std::ceil(top), avg, std_deviation, 100) << '\n';
    }

private:
    tools::histogram<double> m_histogram;
};

//--------------------------------------------------------------------------------------------------------------------------------

void test_periodic_publish_subscribe()
{
    std::cout << "-- periodic publish subscribe --" << '\n';
    auto monitoring = std::make_shared<my_async_observer>();
    auto data_source = std::make_shared<my_subject>("data_source");
    auto histogram_feeder = std::make_shared<my_collector>();

    auto sampler
        = [&data_source](const std::shared_ptr<my_periodic_task_context>& context, const std::string& task_name) -> void
    {
        (void)task_name;

        context->loop_counter += 1;

        // mocked signal
        double signal = std::sin(context->loop_counter.load());

        // emit "signal" as a 'string' event
        data_source->publish(my_topic::external, std::to_string(signal));
    };

    auto startup = [](const std::shared_ptr<my_periodic_task_context>& context, const std::string& task_name) -> void
    {
        (void)task_name;
        context->loop_counter = 0;
    };

    data_source->subscribe(my_topic::external, monitoring);
    data_source->subscribe(my_topic::external, histogram_feeder);

    // "sample" with a 100 ms period
    auto context = std::make_shared<my_periodic_task_context>();
    const auto period = std::chrono::duration<int, std::milli>(100);
    {
        my_periodic_task periodic_task(startup, sampler, context, "periodic task 1", period);

        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));
    }

    histogram_feeder->display_stats();
}

//--------------------------------------------------------------------------------------------------------------------------------


void run_example_pub_sub_and_task()
{
    test_publish_subscribe();
    test_periodic_task();
    test_periodic_publish_subscribe();
}
