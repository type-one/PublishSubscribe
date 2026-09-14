/**
 * @file test_periodic_task.cpp
 * @brief Unit tests for the tools::periodic_task class template.
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

#include "tools/periodic_task.hpp"

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
}

TEST(PeriodicTaskTest, ContextValueIncreasesOverTime)
{
    auto context = std::make_shared<counter_context>();

    auto startup = [](const std::shared_ptr<counter_context>&, const std::string&) { };

    tools::periodic_task<counter_context> task(
        startup, [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); }, context,
        "periodic-test", std::chrono::milliseconds(20));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_GT(context->value(), 1);
}

TEST(PeriodicTaskTest, StartupRoutineRunsOnceBeforeFirstPeriod)
{
    auto context = std::make_shared<counter_context>();
    std::atomic<int> startup_calls { 0 };

    auto startup
        = [&startup_calls](const std::shared_ptr<counter_context>&, const std::string&) { startup_calls.fetch_add(1); };

    tools::periodic_task<counter_context> task(
        startup, [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); }, context,
        "periodic-startup-test", std::chrono::milliseconds(20));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_EQ(startup_calls.load(), 1);
    EXPECT_GT(context->value(), 1);
}

TEST(PeriodicTaskTest, ConstructorSupportsLvalueRvalueAndConversion)
{
    auto context = std::make_shared<counter_context>();

    tools::periodic_task<counter_context>::call_back startup_lvalue
        = [](const std::shared_ptr<counter_context>&, const std::string&) { };
    tools::periodic_task<counter_context>::call_back routine_lvalue
        = [](const std::shared_ptr<counter_context>& ctx, const std::string&) { ctx->increment(); };

    tools::periodic_task<counter_context> task(
        startup_lvalue, routine_lvalue, context, std::string("forwarding-test"), std::chrono::milliseconds(20));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_GT(context->value(), 1);
}
