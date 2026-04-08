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

#pragma once

#if !defined(WORKER_TASK_HPP_)
#define WORKER_TASK_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
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

#include "portable_concurrency/p_execution.hpp"
#include "portable_concurrency/p_future.hpp"

namespace tools
{
    template <typename Context>
    class worker_task;

    /**
     * @brief Executor adapter forwarding tasks to a @ref worker_task instance.
     *
     * This lightweight handle is used with portable_concurrency primitives
     * (for example @ref portable_concurrency::async and future continuations)
     * through the ADL-discovered @ref post customization point below.
     *
     * @tparam Context The worker context type associated with the target worker.
     */
    template <typename Context>
    class worker_task_executor
    {
    public:
        explicit worker_task_executor(worker_task<Context>* owner)
            : m_owner(owner)
        {
        }

    private:
        worker_task<Context>* m_owner = nullptr;

        template <typename Ctx, typename Task>
        friend void post(worker_task_executor<Ctx> exec, Task&& task);
    };

    /**
     * @brief Schedules a task on the worker thread owned by the executor.
     *
     * This function is intentionally placed in namespace @ref tools so it can
     * be found by argument-dependent lookup (ADL), as required by
     * portable_concurrency executor integration.
     *
     * @tparam Context The worker context type.
     * @tparam Task A move-constructible callable compatible with `void()`.
     * @param exec Executor handle that identifies the destination worker.
     * @param task Callable to enqueue for asynchronous execution.
     */
    template <typename Context, typename Task>
    void post(worker_task_executor<Context> exec, Task&& task)
    {
        auto shared_task = std::make_shared<std::decay_t<Task>>(std::forward<Task>(task));
        exec.m_owner->delegate(
            [shared_task](std::shared_ptr<Context>, const std::string&) mutable { (*shared_task)(); });
    }

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

        using executor_type = worker_task_executor<Context>;
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
#if defined(__linux__)
            return reinterpret_cast<void*>(m_task->native_handle());
#else
            return nullptr;
#endif
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

        // Executor adapter so this worker can schedule portable_concurrency continuations.
        [[nodiscard]] executor_type as_executor()
        {
            return executor_type { this };
        }

        // Execute a value-returning function on this worker and get a future for then() chaining.
        template <typename Callable, typename... Args>
        auto delegate_async(
            Callable&& work, Args&&... args) -> decltype(portable_concurrency::async(std::declval<executor_type>(),
                                                 std::forward<Callable>(work), std::declval<std::shared_ptr<Context>>(),
                                                 std::declval<std::string>(), std::forward<Args>(args)...))
        {
            return portable_concurrency::async(
                as_executor(), std::forward<Callable>(work), m_context, m_task_name, std::forward<Args>(args)...);
        }

#if defined(PC_HAS_COROUTINES)
        /**
         * @brief Awaitable that resumes the awaiting coroutine on this worker thread.
         *
         * Usage pattern:
         * `co_await worker.schedule();` to switch execution to the worker context.
         */
        class schedule_awaitable
        {
        public:
            explicit schedule_awaitable(worker_task* owner)
                : m_owner(owner)
            {
            }

            [[nodiscard]] bool await_ready() const noexcept
            {
                return false;
            }

            void await_suspend(portable_concurrency::detail::coroutine_handle<> handle)
            {
                m_owner->delegate([handle](std::shared_ptr<Context>, const std::string&) mutable { handle.resume(); });
            }

            void await_resume() const noexcept
            {
            }

        private:
            worker_task* m_owner = nullptr;
        };

        /**
         * @brief Returns an awaitable to transfer coroutine execution to this worker.
         */
        [[nodiscard]] schedule_awaitable schedule()
        {
            return schedule_awaitable { this };
        }
#endif

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

        tools::sync_object m_work_sync = {};
        tools::sync_queue<call_back> m_work_queue = {};
        std::shared_ptr<Context> m_context;
        std::string m_task_name;
        std::atomic_bool m_stop_task = false;
        std::unique_ptr<std::thread> m_task = {};
    };
}

namespace portable_concurrency
{
    template <typename Context>
    struct is_executor<tools::worker_task_executor<Context>> : std::true_type
    {
    };
}

#endif //  WORKER_TASK_HPP_
