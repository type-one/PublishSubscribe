/**
 * @file worker_task.hpp
 * @brief A worker task class template.
 *
 * This file contains the implementation of the worker_task class template, which
 * provides functionality to delegate tasks and manage their execution.
 *
 * @author Laurent Lardinois
 * @date January 2025
 */

//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois      //
//                                                                             //
// https://github.com/type-one/PublishSubscribeESP32                           //
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

#if !defined(WORKER_TASK_HPP_)
#define WORKER_TASK_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#endif

#if defined(__linux__)
#include <pthread.h>
#endif

#include "tools/non_copyable.hpp"
#include "tools/sync_object.hpp"
#include "tools/sync_queue.hpp"

namespace tools
{
    /**
     * @brief A worker task class template.
     *
     * This class represents a worker task.
     * It provides functionality to delegate tasks and manage their execution.
     *
     * @tparam Context The type of the context object.
     */
    template <typename Context>
    class worker_task : public non_copyable // NOLINT inherits from non copyable/non movable
    {

    public:
        worker_task() = delete;

        using call_back = std::function<void(std::shared_ptr<Context>, const std::string& task_name)>;

        // Forward context and task name at construction to avoid extra copies.
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains constructor arguments.
        template <typename ContextArg, typename NameArg>
            requires std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                         && std::is_constructible_v<std::string, NameArg&&>
        worker_task(ContextArg&& context, NameArg&& task_name)
            : m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_task { std::make_unique<std::thread>(
                  [this]()
                  {
#if defined(__linux__)
                      pthread_setname_np(pthread_self(), m_task_name.c_str());
#endif
                      run_loop();
                  }) }
        {
        }
#else
        // C++17: equivalent constructor constraints expressed with SFINAE.
        template <typename ContextArg, typename NameArg,
            typename = std::enable_if_t<std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                && std::is_constructible_v<std::string, NameArg&&>>>
        worker_task(ContextArg&& context, NameArg&& task_name)
            : m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_task { std::make_unique<std::thread>(
                  [this]()
                  {
#if defined(__linux__)
                      pthread_setname_np(pthread_self(), m_task_name.c_str());
#endif
                      run_loop();
                  }) }
        {
        }
#endif

        ~worker_task()
        {
            m_stop_task.store(true);
            m_work_sync.signal();
            m_task->join();
        }

        // note: native handle allows specific OS calls like setting scheduling policy or setting priority
        [[nodiscard]] void* native_handle() const
        {
            return reinterpret_cast<void*>(m_task->native_handle());
        }

        // rvalue overload: enqueue a pre-built std::function by move.
        void delegate(call_back&& work)
        {
            m_work_queue.emplace(std::move(work));
            m_work_sync.signal();
        }

        // lvalue overload: enqueue a pre-built std::function by copy.
        void delegate(const call_back& work)
        {
            m_work_queue.push(work);
            m_work_sync.signal();
        }

        // perfect forwarding overload for arbitrary callable objects.
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: only participates for callable types convertible to call_back.
        template <typename Callable>
            requires std::is_invocable_r_v<void, Callable&, std::shared_ptr<Context>, const std::string&>
            && std::is_constructible_v<call_back, Callable&&>
        void delegate(Callable&& work)
        {
            m_work_queue.emplace(std::forward<Callable>(work));
            m_work_sync.signal();
        }
#else
        // C++17: equivalent callable constraints expressed with SFINAE.
        template <typename Callable,
            typename
            = std::enable_if_t<std::is_invocable_r_v<void, Callable&, std::shared_ptr<Context>, const std::string&>
                && std::is_constructible_v<call_back, Callable&&>>>
        void delegate(Callable&& work)
        {
            m_work_queue.emplace(std::forward<Callable>(work));
            m_work_sync.signal();
        }
#endif

        // C++17: iterator-pair batch delegate; enqueue all then signal once.
        template <typename InputIt>
        void delegate_range(InputIt first, InputIt last)
        {
            m_work_queue.push_range(first, last);
            m_work_sync.signal();
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: range batch delegate; accepts any input_range of call_back-compatible elements.
        template <std::ranges::input_range Range>
            requires std::is_constructible_v<call_back, std::ranges::range_reference_t<Range>>
        void delegate_range(Range&& range)
        {
            m_work_queue.push_range(std::forward<Range>(range));
            m_work_sync.signal();
        }
#endif

    private:
        void run_loop()
        {
            while (!m_stop_task.load())
            {
                m_work_sync.wait_for_signal();

                while (!m_work_queue.empty())
                {
                    auto work = m_work_queue.front_pop();
                    if (work.has_value())
                    {
                        (*work)(m_context, m_task_name);
                    }
                }
            } // run loop
        }

        call_back m_startup_routine;
        tools::sync_object m_work_sync = {};
        tools::sync_queue<call_back> m_work_queue = {};
        std::shared_ptr<Context> m_context;
        std::string m_task_name;
        std::atomic_bool m_stop_task = false;
        std::unique_ptr<std::thread> m_task = {};
    };
}

#endif //  WORKER_TASK_HPP_
