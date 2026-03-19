/**
 * @file periodic_task.hpp
 * @brief Implementation of a periodic task
 *
 * This file contains the implementation of the periodic_task class, which inherits from base_task
 * and provides functionality to execute a startup routine and a periodic routine at specified intervals.
 *
 * @author Laurent Lardinois
 * @date January 2025
 */

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

#pragma once

#if !defined(PERIODIC_TASK_HPP_)
#define PERIODIC_TASK_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include "tools/linux/linux_sched_deadline.hpp"
#include "tools/non_copyable.hpp"

namespace tools
{
    /**
     * @brief Class representing a periodic task.
     *
     * This class provides functionality to execute a periodic routine at
     * specified intervals.
     *
     * @tparam Context The type of the context object associated with the task.
     */
    template <typename Context>
    class periodic_task : public non_copyable // NOLINT inherits from non copyable/non movable
    {

    public:
        periodic_task() = delete;

        using call_back = std::function<void(std::shared_ptr<Context>, const std::string& task_name)>;

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: perfect-forward callback/context/name into stored members.
        template <typename RoutineArg, typename ContextArg, typename NameArg>
            requires std::is_constructible_v<call_back, RoutineArg&&>
                         && std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                         && std::is_constructible_v<std::string, NameArg&&>
        periodic_task(RoutineArg&& routine, ContextArg&& context, NameArg&& task_name,
            const std::chrono::duration<int, std::micro>& period)
            : m_routine { std::forward<RoutineArg>(routine) }
            , m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_period { period }
            , m_task { std::make_unique<std::thread>([this]() { periodic_call(); }) }
        {
        }
#else
        // C++17: equivalent forwarding constructor constrained via SFINAE.
        template <typename RoutineArg, typename ContextArg, typename NameArg,
            typename = std::enable_if_t<std::is_constructible_v<call_back, RoutineArg&&>
                && std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                && std::is_constructible_v<std::string, NameArg&&>>>
        periodic_task(RoutineArg&& routine, ContextArg&& context, NameArg&& task_name,
            const std::chrono::duration<int, std::micro>& period)
            : m_routine { std::forward<RoutineArg>(routine) }
            , m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_period { period }
            , m_task { std::make_unique<std::thread>([this]() { periodic_call(); }) }
        {
        }
#endif

        ~periodic_task()
        {
            m_stop_task.store(true);
            m_task->join();
        }

    private:
        void periodic_call()
        {
            const auto start_time = std::chrono::high_resolution_clock::now();
            auto deadline = start_time + m_period;

            const auto earliest_deadline_enabled = set_earliest_deadline_scheduling(start_time, m_period);

            while (!m_stop_task.load())
            {
                // active wait loop
                std::chrono::high_resolution_clock::time_point current_time;
                do
                {
                    current_time = std::chrono::high_resolution_clock::now();
                } while (deadline > current_time);

                // execute given periodic function
                m_routine(m_context, m_task_name);

                // compute next deadline
                deadline += m_period;

                current_time = std::chrono::high_resolution_clock::now();

                // wait period
                if (deadline > current_time)
                {
                    const auto remaining_time
                        = std::chrono::duration_cast<std::chrono::microseconds>(deadline - current_time);
                    // wait between 90% and 96% of the remaining time depending on scheduling mode
                    const double ratio = (earliest_deadline_enabled) ? 0.96 : 0.9;

                    // sleep until we are close to the deadline
                    const auto sleep_time
                        = std::chrono::duration<int, std::micro>(static_cast<int>(ratio * remaining_time.count()));
                    std::this_thread::sleep_for(sleep_time);

                } // end if wait period needed
            } // periodic task loop
        }

        call_back m_routine;
        std::shared_ptr<Context> m_context;
        std::string m_task_name;
        std::chrono::duration<int, std::micro> m_period;
        std::unique_ptr<std::thread> m_task = {};
        std::atomic_bool m_stop_task = false;
    };
}

#endif //  PERIODIC_TASK_HPP_
