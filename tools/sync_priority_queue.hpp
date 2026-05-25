/**
 * @file sync_priority_queue.hpp
 * @brief A thread-safe priority queue implementation.
 *
 * This file contains the definition of the sync_priority_queue class, which provides a thread-safe
 * priority queue with various methods to manipulate the queue.
 *
 * @author Laurent Lardinois
 * @date May 2026
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

#if !defined(SYNC_PRIORITY_QUEUE_HPP_)
#define SYNC_PRIORITY_QUEUE_HPP_

#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <type_traits>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#endif

#include "tools/non_copyable.hpp"

namespace tools
{
    /**
     * @brief A thread-safe priority queue implementation.
     *
     * Default behavior is min-heap (lowest value popped first) via std::greater<T>.
     *
     * @tparam T The type of elements stored in the queue.
     * @tparam Compare Comparator used by std::priority_queue.
     */
    template <typename T, typename Compare = std::greater<T>>
    class sync_priority_queue : public non_copyable // NOLINT inherits from non copyable/non movable
    {
    public:
        sync_priority_queue() = default;
        ~sync_priority_queue() = default;

        struct thread_safe
        {
            static constexpr bool value = true;
        };

        void push(const T& elem)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.push(elem);
        }

        void push(T&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.push(std::move(elem));
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        template <typename U>
            requires std::is_constructible_v<T, U>
        void push(U&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.push(std::forward<U>(elem));
        }

        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.emplace(std::forward<Args>(args)...);
        }
#else
        template <typename U, typename = std::enable_if_t<std::is_constructible_v<T, U>>>
        void push(U&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.push(std::forward<U>(elem));
        }

        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_priority_queue.emplace(std::forward<Args>(args)...);
        }
#endif

        void pop()
        {
            std::unique_lock guard(m_mutex);
            if (!m_priority_queue.empty())
            {
                m_priority_queue.pop();
            }
        }

        [[nodiscard]] std::optional<T> top() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_priority_queue.empty())
            {
                item = m_priority_queue.top();
            }
            return item;
        }

        [[nodiscard]] std::optional<T> top_pop()
        {
            std::optional<T> item;
            std::unique_lock guard(m_mutex);
            if (!m_priority_queue.empty())
            {
                item = m_priority_queue.top();
                m_priority_queue.pop();
            }
            return item;
        }

        // Queue-compatible aliases to integrate with async_observer.
        [[nodiscard]] std::optional<T> front() const
        {
            return top();
        }

        [[nodiscard]] std::optional<T> back() const
        {
            return top();
        }

        [[nodiscard]] std::optional<T> front_pop()
        {
            return top_pop();
        }

        [[nodiscard]] bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_priority_queue.empty();
        }

        [[nodiscard]] std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_priority_queue.size();
        }

        template <typename InputIt>
        void push_range(InputIt first, InputIt last)
        {
            std::unique_lock guard(m_mutex);
            for (; first != last; ++first)
            {
                m_priority_queue.push(T(*first));
            }
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        template <std::ranges::input_range Range>
            requires std::is_constructible_v<T, std::ranges::range_value_t<Range>>
        void push_range(Range&& range)
        {
            std::unique_lock guard(m_mutex);
            for (auto&& elem : range)
            {
                m_priority_queue.push(T(std::forward<decltype(elem)>(elem)));
            }
        }
#endif

    private:
        std::priority_queue<T, std::vector<T>, Compare> m_priority_queue;
        mutable std::shared_mutex m_mutex;
    };

    template <typename T>
    using sync_max_priority_queue = sync_priority_queue<T, std::less<T>>;
}

#endif //  SYNC_PRIORITY_QUEUE_HPP_
