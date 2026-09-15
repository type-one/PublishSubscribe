/**
 * @file data_task.hpp
 * @brief A bounded asynchronous data-processing task.
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
//-----------------------------------------------------------------------------//

#pragma once

#if !defined(DATA_TASK_HPP_)
#define DATA_TASK_HPP_

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

#include "tools/non_copyable.hpp"
#include "tools/sync_object.hpp"
#include "tools/sync_ring_vector.hpp"

namespace tools
{
    /**
     * @brief Processes submitted data values sequentially on a dedicated thread.
     *
     * The task owns a bounded FIFO queue. A submission made while the queue is
     * full is rejected and leaves the already queued data unchanged.
     *
     * @tparam Context Shared context made available to each callback.
     * @tparam DataType Trivially copyable data type carried by the task.
     */
    template <typename Context, typename DataType>
    class data_task : public non_copyable // NOLINT inherits from non copyable/non movable
    {
        static_assert(std::is_standard_layout_v<DataType>, "DataType must have standard layout");
        static_assert(std::is_trivial_v<DataType>, "DataType must be trivial");

    public:
        using timeout_type = std::chrono::microseconds;
        using call_back = std::function<void(const std::shared_ptr<Context>& context, const std::string& task_name)>;
        using data_call_back = std::function<void(
            const std::shared_ptr<Context>& context, const DataType& data, const std::string& task_name)>;

        data_task() = delete;

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        template <typename StartupArg, typename ProcessArg, typename ContextArg, typename NameArg>
            requires std::is_constructible_v<call_back, StartupArg&&>
                         && std::is_constructible_v<data_call_back, ProcessArg&&>
                         && std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                         && std::is_constructible_v<std::string, NameArg&&>
        data_task(StartupArg&& startup_routine, ProcessArg&& process_routine, ContextArg&& context,
            std::size_t data_queue_depth, NameArg&& task_name, timeout_type data_timeout = timeout_type::max())
            : m_startup_routine { std::forward<StartupArg>(startup_routine) }
            , m_process_routine { std::forward<ProcessArg>(process_routine) }
            , m_data_queue { data_queue_depth }
            , m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_data_timeout { data_timeout }
            , m_task { std::make_unique<std::thread>([this]() { run_loop(); }) }
        {
        }
#else
        template <typename StartupArg, typename ProcessArg, typename ContextArg, typename NameArg,
            typename = std::enable_if_t<std::is_constructible_v<call_back, StartupArg&&>
                && std::is_constructible_v<data_call_back, ProcessArg&&>
                && std::is_constructible_v<std::shared_ptr<Context>, ContextArg&&>
                && std::is_constructible_v<std::string, NameArg&&>>>
        data_task(StartupArg&& startup_routine, ProcessArg&& process_routine, ContextArg&& context,
            std::size_t data_queue_depth, NameArg&& task_name, timeout_type data_timeout = timeout_type::max())
            : m_startup_routine { std::forward<StartupArg>(startup_routine) }
            , m_process_routine { std::forward<ProcessArg>(process_routine) }
            , m_data_queue { data_queue_depth }
            , m_context { std::forward<ContextArg>(context) }
            , m_task_name { std::forward<NameArg>(task_name) }
            , m_data_timeout { data_timeout }
            , m_task { std::make_unique<std::thread>([this]() { run_loop(); }) }
        {
        }
#endif

        ~data_task()
        {
            m_stop_task.store(true);
            m_data_sync.signal();
            m_task->join();
        }

        /**
         * @brief Returns the native handle of the task thread where available.
         * @return Native task handle on Linux; otherwise `nullptr`.
         */
        [[nodiscard]] void* native_handle() const
        {
#if defined(__linux__)
            return reinterpret_cast<void*>(m_task->native_handle());
#else
            return nullptr;
#endif
        }

        /**
         * @brief Attempts to enqueue data for asynchronous processing.
         * @param data Data value to enqueue.
         * @return `true` when enqueued; `false` when the bounded queue is full.
         */
        bool submit(const DataType& data)
        {
            const auto submitted = m_data_queue.push(data);
            if (submitted)
            {
                m_data_sync.signal();
            }
            return submitted;
        }

        /**
         * @brief Attempts to enqueue a movable data value for asynchronous processing.
         * @param data Data value to enqueue.
         * @return `true` when enqueued; `false` when the bounded queue is full.
         */
        bool submit(DataType&& data)
        {
            const auto submitted = m_data_queue.push(std::move(data));
            if (submitted)
            {
                m_data_sync.signal();
            }
            return submitted;
        }

    private:
        void run_loop()
        {
#if defined(__linux__)
            pthread_setname_np(pthread_self(), m_task_name.c_str());
#endif
            m_startup_routine(m_context, m_task_name);

            while (!m_stop_task.load())
            {
                if (m_data_timeout == timeout_type::max())
                {
                    m_data_sync.wait_for_signal();
                }
                else
                {
                    m_data_sync.wait_for_signal(m_data_timeout);
                }

                while (!m_data_queue.empty())
                {
                    const auto data = m_data_queue.front_pop();
                    if (data.has_value())
                    {
                        m_process_routine(m_context, *data, m_task_name);
                    }
                }
            }
        }

        call_back m_startup_routine;
        data_call_back m_process_routine;
        sync_object m_data_sync = {};
        sync_ring_vector<DataType> m_data_queue;
        std::shared_ptr<Context> m_context;
        std::string m_task_name;
        timeout_type m_data_timeout;
        std::atomic_bool m_stop_task = false;
        std::unique_ptr<std::thread> m_task = {};
    };
}

#endif // DATA_TASK_HPP_
