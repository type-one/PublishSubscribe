/**
 * @file time_list.hpp
 * @brief Chronologically sorted timestamp/value helper based on std::priority_queue.
 *
 * This file defines tools::time_list, a non-thread-safe container that stores
 * <timestamp, value> pairs and always exposes the earliest timestamp first.
 *
 * Supported timestamp types:
 * - std::chrono::time_point<Clock, Duration>
 * - integral types (signed/unsigned)
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

#if !defined(TIME_LIST_HPP_)
#define TIME_LIST_HPP_

#include <chrono>
#include <cstddef>
#include <optional>
#include <queue>
#include <type_traits>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <concepts>
#endif

namespace tools
{
    namespace detail
    {
        template <typename TType>
        struct is_std_time_point : std::false_type
        {
        };

        template <typename TClock, typename TDuration>
        struct is_std_time_point<std::chrono::time_point<TClock, TDuration>> : std::true_type
        {
        };

        template <typename TType>
        struct is_valid_timestamp : std::integral_constant<bool,
                                        std::is_integral<typename std::decay<TType>::type>::value
                                            || is_std_time_point<typename std::decay<TType>::type>::value>
        {
        };
    } // namespace detail

    /**
     * @brief Non-thread-safe chronological list based on std::priority_queue.
     *
     * Stores entries as <timestamp, value>. The earliest timestamp is always at
     * the top of the queue.
     *
     * @tparam TTimestamp Timestamp type (chrono::time_point or integral).
     * @tparam TValue Value type.
     */
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    template <typename TTimestamp, typename TValue>
        requires detail::is_valid_timestamp<TTimestamp>::value
    class time_list
    {
#else
    template <typename TTimestamp, typename TValue>
    class time_list
    {
        static_assert(detail::is_valid_timestamp<TTimestamp>::value,
            "TTimestamp must be std::chrono::time_point<...> or an integral type");
#endif

    public:
        using timestamp_type = TTimestamp;
        using value_type = TValue;
        using entry_type = std::pair<timestamp_type, value_type>;

        struct entry_compare
        {
            [[nodiscard]] bool operator()(const entry_type& lhs, const entry_type& rhs) const
            {
                // std::priority_queue keeps "largest" on top; invert for earliest-first behavior.
                return lhs.first > rhs.first;
            }
        };

        using queue_type = std::priority_queue<entry_type, std::vector<entry_type>, entry_compare>;

        void push(const timestamp_type& timestamp_value, const value_type& payload_value)
        {
            m_queue.emplace(timestamp_value, payload_value);
        }

        void push(timestamp_type&& timestamp_value, value_type&& payload_value)
        {
            m_queue.emplace(std::move(timestamp_value), std::move(payload_value));
        }

        template <typename TT, typename TV>
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
            requires std::constructible_from<timestamp_type, TT> && std::constructible_from<value_type, TV>
#endif
        auto push(TT&& timestamp_value, TV&& payload_value)
#if !((__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L)))
            -> typename std::enable_if<std::is_constructible<timestamp_type, TT>::value
                    && std::is_constructible<value_type, TV>::value,
                void>::type
#endif
        {
            m_queue.emplace(std::forward<TT>(timestamp_value), std::forward<TV>(payload_value));
        }

        template <typename... TArgs>
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
            requires std::constructible_from<value_type, TArgs...>
#endif
        auto emplace(const timestamp_type& timestamp_value, TArgs&&... value_args)
#if !((__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L)))
            -> typename std::enable_if<std::is_constructible<value_type, TArgs...>::value, void>::type
#endif
        {
            m_queue.emplace(timestamp_value, value_type(std::forward<TArgs>(value_args)...));
        }

        [[nodiscard]] std::optional<entry_type> top() const
        {
            if (m_queue.empty())
            {
                return std::nullopt;
            }
            return m_queue.top();
        }

        void pop()
        {
            if (!m_queue.empty())
            {
                m_queue.pop();
            }
        }

        [[nodiscard]] std::optional<entry_type> top_pop()
        {
            if (m_queue.empty())
            {
                return std::nullopt;
            }

            entry_type first_entry = m_queue.top();
            m_queue.pop();
            return first_entry;
        }

        [[nodiscard]] bool empty() const
        {
            return m_queue.empty();
        }

        [[nodiscard]] std::size_t size() const
        {
            return m_queue.size();
        }

        void clear()
        {
            queue_type empty_queue;
            m_queue.swap(empty_queue);
        }

        [[nodiscard]] std::vector<entry_type> snapshot_sorted() const
        {
            std::vector<entry_type> result_values;
            result_values.reserve(m_queue.size());

            queue_type queue_copy = m_queue;
            while (!queue_copy.empty())
            {
                result_values.push_back(queue_copy.top());
                queue_copy.pop();
            }

            return result_values;
        }

    private:
        queue_type m_queue;
    };

} // namespace tools

#endif // TIME_LIST_HPP_
