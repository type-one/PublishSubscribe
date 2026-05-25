/**
 * @file sync_time_list.hpp
 * @brief Thread-safe chronological timestamp/value helper based on tools::time_list.
 *
 * This file defines tools::sync_time_list, a thread-safe adapter over
 * tools::time_list that synchronizes all operations.
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

#if !defined(SYNC_TIME_LIST_HPP_)
#define SYNC_TIME_LIST_HPP_

#include <cstddef>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <concepts>
#endif

#include "tools/non_copyable.hpp"
#include "tools/time_list.hpp"

namespace tools
{
    /**
     * @brief Thread-safe adapter over tools::time_list.
     *
     * @tparam TTimestamp Timestamp type accepted by tools::time_list.
     * @tparam TValue Value type associated with each timestamp.
     */
    template <typename TTimestamp, typename TValue>
    class sync_time_list : public non_copyable // NOLINT non-copyable by design
    {
    public:
        using base_type = time_list<TTimestamp, TValue>;
        using timestamp_type = typename base_type::timestamp_type;
        using value_type = typename base_type::value_type;
        using entry_type = typename base_type::entry_type;

        struct thread_safe
        {
            static constexpr bool value = true;
        };

        sync_time_list() = default;
        ~sync_time_list() = default;

        void push(const timestamp_type& timestamp_value, const value_type& payload_value)
        {
            std::unique_lock guard(m_mutex);
            m_list.push(timestamp_value, payload_value);
        }

        void push(timestamp_type&& timestamp_value, value_type&& payload_value)
        {
            std::unique_lock guard(m_mutex);
            m_list.push(std::move(timestamp_value), std::move(payload_value));
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
            std::unique_lock guard(m_mutex);
            m_list.push(std::forward<TT>(timestamp_value), std::forward<TV>(payload_value));
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
            std::unique_lock guard(m_mutex);
            m_list.emplace(timestamp_value, std::forward<TArgs>(value_args)...);
        }

        [[nodiscard]] std::optional<entry_type> top() const
        {
            std::shared_lock guard(m_mutex);
            return m_list.top();
        }

        void pop()
        {
            std::unique_lock guard(m_mutex);
            m_list.pop();
        }

        [[nodiscard]] std::optional<entry_type> top_pop()
        {
            std::unique_lock guard(m_mutex);
            return m_list.top_pop();
        }

        [[nodiscard]] bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_list.empty();
        }

        [[nodiscard]] std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_list.size();
        }

        void clear()
        {
            std::unique_lock guard(m_mutex);
            m_list.clear();
        }

        [[nodiscard]] std::vector<entry_type> snapshot_sorted() const
        {
            std::shared_lock guard(m_mutex);
            return m_list.snapshot_sorted();
        }

    private:
        base_type m_list;
        mutable std::shared_mutex m_mutex;
    };

} // namespace tools

#endif // SYNC_TIME_LIST_HPP_
