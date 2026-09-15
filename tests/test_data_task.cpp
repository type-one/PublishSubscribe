/**
 * @file test_data_task.cpp
 * @brief Unit tests for the tools::data_task class template.
 *
 * @author Laurent Lardinois and Codex
 * @date September 2026
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "tools/data_task.hpp"

namespace
{
    class data_context
    {
    public:
        void add(int value)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_values.push_back(value);
        }

        [[nodiscard]] std::vector<int> values() const
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            return m_values;
        }

    private:
        mutable std::mutex m_mutex;
        std::vector<int> m_values;
    };

    template <typename Predicate>
    bool wait_until(Predicate predicate, std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (predicate())
            {
                return true;
            }
            std::this_thread::yield();
        }
        return predicate();
    }
}

TEST(DataTaskTest, RunsStartupAndProcessesSubmittedDataInOrder)
{
    auto context = std::make_shared<data_context>();
    std::atomic<int> startup_calls { 0 };

    tools::data_task<data_context, int> task([&startup_calls](const std::shared_ptr<data_context>&, const std::string&)
        { startup_calls.fetch_add(1); },
        [](const std::shared_ptr<data_context>& task_context, const int& data, const std::string&)
        { task_context->add(data); }, context, 3U, "data-task-test");

    EXPECT_TRUE(task.submit(1));
    EXPECT_TRUE(task.submit(2));
    EXPECT_TRUE(task.submit(3));

    EXPECT_TRUE(wait_until([&context]() { return context->values().size() == 3U; }));
    EXPECT_EQ(context->values(), (std::vector<int> { 1, 2, 3 }));
    EXPECT_EQ(startup_calls.load(), 1);
}

TEST(DataTaskTest, RejectsSubmissionWhenBoundedQueueIsFull)
{
    auto context = std::make_shared<data_context>();
    std::atomic_bool allow_startup_to_finish { false };

    tools::data_task<data_context, int> task(
        [&allow_startup_to_finish](const std::shared_ptr<data_context>&, const std::string&)
        {
            while (!allow_startup_to_finish.load())
            {
                std::this_thread::yield();
            }
        },
        [](const std::shared_ptr<data_context>& task_context, const int& data, const std::string&)
        { task_context->add(data); }, context, 1U, "bounded-data-task");

    EXPECT_TRUE(task.submit(1));
    EXPECT_FALSE(task.submit(2));

    allow_startup_to_finish.store(true);
    EXPECT_TRUE(wait_until([&context]() { return context->values().size() == 1U; }));
    EXPECT_EQ(context->values(), (std::vector<int> { 1 }));
}
