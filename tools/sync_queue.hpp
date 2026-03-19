/**
 * @file sync_queue.hpp
 * @brief A thread-safe queue implementation.
 *
 * This file contains the definition of the sync_queue class, which provides a thread-safe
 * queue with various methods to manipulate the queue.
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

#if !defined(SYNC_QUEUE_HPP_)
#define SYNC_QUEUE_HPP_

#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <type_traits>
#include <utility>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#endif

#include "tools/non_copyable.hpp"

namespace tools
{
    /**
     * @brief A thread-safe queue implementation.
     *
     * This class provides a thread-safe queue with various methods to manipulate the queue.
     * It inherits from non_copyable to prevent copying and moving.
     *
     * @tparam T The type of elements stored in the queue.
     */
    template <typename T>
    class sync_queue : public non_copyable // NOLINT inherits from non copyable/non movable
    {
    public:
        sync_queue() = default;
        ~sync_queue() = default;

        void push(const T& elem)
        {
            std::unique_lock guard(m_mutex);
            m_queue.push(elem);
        }

        // rvalue overload: moves an already-constructed element into the queue
        void push(T&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_queue.push(std::move(elem));
        }

        // perfect forwarding: constructs T in-place from arbitrary constructor arguments
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains the template to valid T constructors
        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_queue.emplace(std::forward<Args>(args)...);
        }
#else
        // C++17: std::enable_if_t provides equivalent SFINAE constraint
        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_queue.emplace(std::forward<Args>(args)...);
        }
#endif

        // C++17: iterator-pair batch push — inserts [first, last) under a single lock
        template <typename InputIt>
        void push_range(InputIt first, InputIt last)
        {
            std::unique_lock guard(m_mutex);
            for (; first != last; ++first)
            {
                m_queue.emplace(*first); // emplace handles move iterators transparently
            }
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: range overload — accepts any input_range (vector, span, views, ...) under one lock
        template <std::ranges::input_range Range>
            requires std::is_constructible_v<T, std::ranges::range_value_t<Range>>
        void push_range(Range&& range)
        {
            std::unique_lock guard(m_mutex);
            for (auto&& elem : range)
            {
                m_queue.push(std::forward<decltype(elem)>(elem));
            }
        }
#endif

        void pop()
        {
            std::unique_lock guard(m_mutex);
            if (!m_queue.empty())
            {
                m_queue.pop();
            }
        }

        [[nodiscard]] std::optional<T> front_pop()
        {
            std::optional<T> item;
            std::unique_lock guard(m_mutex);
            if (!m_queue.empty())
            {
                item = m_queue.front();
                m_queue.pop();
            }
            return item;
        }

        [[nodiscard]] std::optional<T> front() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_queue.empty())
            {
                item = m_queue.front();
            }
            return item;
        }

        [[nodiscard]] std::optional<T> back() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_queue.empty())
            {
                item = m_queue.back();
            }
            return item;
        }

        [[nodiscard]] bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_queue.empty();
        }

        [[nodiscard]] std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_queue.size();
        }

    private:
        std::queue<T> m_queue;
        mutable std::shared_mutex m_mutex;
    };
}

#endif //  SYNC_QUEUE_HPP_
