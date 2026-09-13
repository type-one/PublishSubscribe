/**
 * @file test_worker_task.cpp
 * @brief Unit tests for the tools::worker_task class template.
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
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "tools/worker_task.hpp"

namespace
{
    class counter_context
    {
    public:
        void increment()
        {
            m_value.fetch_add(1);
        }

        [[nodiscard]] int value() const
        {
            return m_value.load();
        }

    private:
        std::atomic<int> m_value { 0 };
    };

    // polls until predicate is true or the timeout elapses
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

TEST(WorkerTaskTest, DelegateExecutesWorkOnWorkerThread)
{
    auto context = std::make_shared<counter_context>();
    tools::worker_task<counter_context> worker(context, "worker-test");

    worker.delegate([](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); });

    EXPECT_TRUE(wait_until([&context]() { return context->value() == 1; }));
}

TEST(WorkerTaskTest, DelegateSupportsLvalueRvalueAndConversion)
{
    auto context = std::make_shared<counter_context>();
    tools::worker_task<counter_context> worker(context, "worker-test");

    tools::worker_task<counter_context>::call_back work_lvalue
        = [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); };

    worker.delegate(work_lvalue);
    worker.delegate([](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); });

    EXPECT_TRUE(wait_until([&context]() { return context->value() == 2; }));
}

TEST(WorkerTaskTest, DelegateRangeEnqueuesAllTasks)
{
    auto context = std::make_shared<counter_context>();
    tools::worker_task<counter_context> worker(context, "worker-test");

    using call_back = tools::worker_task<counter_context>::call_back;
    const std::vector<call_back> tasks = {
        [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); },
        [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); },
        [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); },
    };

    worker.delegate_range(tasks.begin(), tasks.end());

    EXPECT_TRUE(wait_until([&context]() { return context->value() == 3; }));
}

TEST(WorkerTaskTest, DelegateAsyncReturnsFutureWithResult)
{
    auto context = std::make_shared<counter_context>();
    tools::worker_task<counter_context> worker(context, "worker-test");

    auto result_future = worker.delegate_async(
        [](const std::shared_ptr<counter_context>&, const std::string&, int value) { return value * 2; }, 21);

    EXPECT_EQ(result_future.get(), 42);
}
