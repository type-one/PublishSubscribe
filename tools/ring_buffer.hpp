/**
 * @file ring_buffer.hpp
 * @brief A header file defining a thread-unsafe ring buffer class template with a fixed capacity.
 *
 * This file contains the definition of the ring_buffer class template, which provides
 * a circular buffer implementation with a fixed capacity. The ring buffer supports
 * operations such as push, pop, front, back, and various utility functions.
 *
 * @author Laurent Lardinois
 *
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

#if !defined(RING_BUFFER_HPP_)
#define RING_BUFFER_HPP_

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#include <span>
#endif

namespace tools
{
    /**
     * @brief A class representing a ring buffer with a fixed capacity.
     *
     * This class provides a circular buffer implementation with a fixed capacity.
     *
     * @tparam T The type of elements stored in the ring buffer.
     * @tparam Capacity The maximum number of elements the ring buffer can hold.
     */
    template <typename T, std::size_t Capacity>
    class ring_buffer
    {
    public:
        static_assert(Capacity > 0U, "ring_buffer Capacity must be greater than 0");

        struct push_range_overwrite_result
        {
            std::size_t inserted = 0U;
            std::size_t overwritten = 0U;
        };

        struct thread_safe
        {
            static constexpr bool value = false;
        };

        ring_buffer() = default;
        ~ring_buffer() = default;

        ring_buffer(const ring_buffer& other)
            : m_ring_buffer { other.m_ring_buffer }
            , m_push_index { other.m_push_index }
            , m_pop_index { other.m_pop_index }
            , m_last_index { other.m_last_index }
            , m_size { other.m_size }
        {
        }

        ring_buffer(ring_buffer&& other) noexcept
            : m_ring_buffer { std::move(other.m_ring_buffer) }
            , m_push_index { std::move(other.m_push_index) }
            , m_pop_index { std::move(other.m_pop_index) }
            , m_last_index { std::move(other.m_last_index) }
            , m_size { std::move(other.m_size) }
        {
        }

        ring_buffer& operator=(const ring_buffer& other)
        {
            if (this != &other)
            {
                m_ring_buffer = other.m_ring_buffer;
                m_push_index = other.m_push_index;
                m_pop_index = other.m_pop_index;
                m_last_index = other.m_last_index;
                m_size = other.m_size;
            }

            return *this;
        }

        ring_buffer& operator=(ring_buffer&& other) noexcept
        {
            if (this != &other)
            {
                m_ring_buffer = std::move(other.m_ring_buffer);
                m_push_index = std::move(other.m_push_index);
                m_pop_index = std::move(other.m_pop_index);
                m_last_index = std::move(other.m_last_index);
                m_size = std::move(other.m_size);
            }

            return *this;
        }

        bool push(const T& elem)
        {
            return write_value(elem, overflow_policy::reject) != write_status::rejected;
        }

        // rvalue overload: moves an already-constructed element into the buffer
        bool push(T&& elem)
        {
            return write_value(std::move(elem), overflow_policy::reject) != write_status::rejected;
        }

        // overwrite variant: when full, evicts oldest entry and inserts the new one.
        // returns true if an old value was overwritten, false if inserted without eviction.
        bool push_overwrite(const T& elem)
        {
            return write_value(elem, overflow_policy::overwrite) == write_status::overwritten;
        }

        // rvalue overload: overwrite behavior
        bool push_overwrite(T&& elem)
        {
            return write_value(std::move(elem), overflow_policy::overwrite) == write_status::overwritten;
        }

        // perfect forwarding: constructs T in-place from arbitrary constructor arguments
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains the template to valid T constructors
        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        bool emplace(Args&&... args)
        {
            return write_value(T(std::forward<Args>(args)...), overflow_policy::reject) != write_status::rejected;
        }

        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        bool emplace_overwrite(Args&&... args)
        {
            return write_value(T(std::forward<Args>(args)...), overflow_policy::overwrite) == write_status::overwritten;
        }
#else
        // C++17: std::enable_if_t provides equivalent SFINAE constraint
        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        bool emplace(Args&&... args)
        {
            return write_value(T(std::forward<Args>(args)...), overflow_policy::reject) != write_status::rejected;
        }

        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        bool emplace_overwrite(Args&&... args)
        {
            return write_value(T(std::forward<Args>(args)...), overflow_policy::overwrite) == write_status::overwritten;
        }
#endif

        // C++17: iterator-pair batch push that stops when the ring buffer is full
        template <typename InputIt>
        std::size_t push_range(InputIt first, InputIt last)
        {
            std::size_t inserted = 0U;
            for (; (first != last) && !full(); ++first)
            {
                if (!push(*first))
                {
                    break;
                }

                ++inserted;
            }
            return inserted;
        }

        // C++17: iterator-pair batch push with overwrite behavior
        template <typename InputIt>
        push_range_overwrite_result push_range_overwrite(InputIt first, InputIt last)
        {
            push_range_overwrite_result result {};
            for (; first != last; ++first)
            {
                const bool overwritten = push_overwrite(*first);
                ++result.inserted;
                result.overwritten += overwritten ? 1U : 0U;
            }

            return result;
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: range batch push that stops when the ring buffer is full
        template <std::ranges::input_range Range>
            requires std::is_constructible_v<T, std::ranges::range_reference_t<Range>>
        std::size_t push_range(Range&& range)
        {
            std::size_t inserted = 0U;
            for (auto&& elem : range)
            {
                if (!push(std::forward<decltype(elem)>(elem)))
                {
                    break;
                }

                ++inserted;
            }
            return inserted;
        }

        // C++20: range batch push with overwrite behavior
        template <std::ranges::input_range Range>
            requires std::is_constructible_v<T, std::ranges::range_reference_t<Range>>
        push_range_overwrite_result push_range_overwrite(Range&& range)
        {
            push_range_overwrite_result result {};
            for (auto&& elem : range)
            {
                const bool overwritten = push_overwrite(std::forward<decltype(elem)>(elem));
                ++result.inserted;
                result.overwritten += overwritten ? 1U : 0U;
            }

            return result;
        }
#endif

        void pop()
        {
            if (!empty())
            {
                m_pop_index = next_index(m_pop_index);
                --m_size;
            }
        }

        // C++17: iterator-pair batch pop that stops when the ring buffer is empty
        template <typename OutputIt>
        std::size_t pop_range(OutputIt first, OutputIt last)
        {
            std::size_t popped = 0U;
            for (; (first != last) && !empty(); ++first)
            {
                *first = std::move(m_ring_buffer[m_pop_index]);
                m_pop_index = next_index(m_pop_index);
                --m_size;
                ++popped;
            }
            return popped;
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: span-based batch pop into contiguous storage
        std::size_t pop_range(std::span<T> out)
        {
            std::size_t popped = 0U;
            for (auto& elem : out)
            {
                if (empty())
                {
                    break;
                }

                elem = std::move(m_ring_buffer[m_pop_index]);
                m_pop_index = next_index(m_pop_index);
                --m_size;
                ++popped;
            }
            return popped;
        }
#endif

        void clear()
        {
            m_push_index = 0U;
            m_pop_index = 0U;
            m_last_index = 0U;
            m_size = 0U;
        }

        T front() const
        {
            return m_ring_buffer[m_pop_index];
        }

        T back() const
        {
            return m_ring_buffer[m_last_index];
        }

        [[nodiscard]] bool empty() const
        {
            return m_size == 0U;
        }

        [[nodiscard]] bool full() const
        {
            return m_size >= Capacity;
        }

        [[nodiscard]] std::size_t size() const
        {
            return m_size;
        }

        [[nodiscard]] constexpr std::size_t capacity() const
        {
            return Capacity;
        }

    private:
        enum class overflow_policy
        {
            reject,
            overwrite
        };

        enum class write_status
        {
            rejected,
            inserted,
            overwritten
        };

        template <typename U>
        write_status write_value(U&& elem, overflow_policy policy)
        {
            bool overwritten = false;

            if (full())
            {
                if (policy == overflow_policy::reject)
                {
                    return write_status::rejected;
                }

                // keep most recent history by evicting the oldest item
                m_pop_index = next_index(m_pop_index);
                if (m_size > 0U)
                {
                    --m_size;
                }
                overwritten = true;
            }

            m_ring_buffer[m_push_index] = std::forward<U>(elem);
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;

            return overwritten ? write_status::overwritten : write_status::inserted;
        }

        [[nodiscard]] constexpr std::size_t next_index(std::size_t index) const
        {
            return ((index + 1U) % Capacity);
        }

        std::array<T, Capacity> m_ring_buffer;
        std::size_t m_push_index = 0U;
        std::size_t m_pop_index = 0U;
        std::size_t m_last_index = 0U;
        std::size_t m_size = 0U;
    };
}

#endif //  RING_BUFFER_HPP_
