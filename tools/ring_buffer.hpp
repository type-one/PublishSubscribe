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

        void push(const T& elem)
        {
            m_ring_buffer[m_push_index] = elem;
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
        }

        // rvalue overload: moves an already-constructed element into the buffer
        void push(T&& elem)
        {
            m_ring_buffer[m_push_index] = std::move(elem);
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
        }

        // perfect forwarding: constructs T in-place from arbitrary constructor arguments
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains the template to valid T constructors
        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        void emplace(Args&&... args)
        {
            m_ring_buffer[m_push_index] = T(std::forward<Args>(args)...);
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
        }
#else
        // C++17: std::enable_if_t provides equivalent SFINAE constraint
        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        void emplace(Args&&... args)
        {
            m_ring_buffer[m_push_index] = T(std::forward<Args>(args)...);
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
        }
#endif

        // C++17: iterator-pair batch push that stops when the ring buffer is full
        template <typename InputIt>
        std::size_t push_range(InputIt first, InputIt last)
        {
            std::size_t inserted = 0U;
            for (; (first != last) && !full(); ++first)
            {
                emplace(*first);
                ++inserted;
            }
            return inserted;
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
                if (full())
                {
                    break;
                }

                emplace(std::forward<decltype(elem)>(elem));
                ++inserted;
            }
            return inserted;
        }
#endif

        void pop()
        {
            m_pop_index = next_index(m_pop_index);
            --m_size;
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
            return m_push_index == m_pop_index;
        }

        [[nodiscard]] bool full() const
        {
            return next_index(m_push_index) == m_pop_index;
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
