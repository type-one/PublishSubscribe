/**
 * @file ring_vector.hpp
 * @brief A thread-unsafe ring buffer implementation using a vector.
 *
 * This file contains the definition of the ring_vector class template, which provides
 * a ring buffer (circular buffer) implementation using a vector. It supports basic
 * operations such as push, pop, front, back, and resizing.
 *
 * @author Laurent Lardinois
 * @date February 2025
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

#if !defined(RING_VECTOR_HPP_)
#define RING_VECTOR_HPP_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#include <span>
#endif

namespace tools
{

    /**
     * @brief A thread-unsafe ring buffer implementation using a vector.
     *
     * This class provides a ring buffer (circular buffer) implementation using a vector.
     * It supports basic operations such as push, pop, front, back, and resizing.
     *
     * @tparam T The type of elements stored in the ring buffer.
     */
    template <typename T>
    class ring_vector
    {
    public:
        struct thread_safe
        {
            static constexpr bool value = false;
        };

        ring_vector() = delete;
        ~ring_vector() = default;

        /**
         * @brief Constructs a ring_vector with the specified capacity.
         *
         * @param capacity The capacity of the ring_vector
         */
        explicit ring_vector(std::size_t capacity)
            : m_ring_vector(capacity)
            , m_capacity(capacity)
        {
        }

        /**
         * @brief Copy constructor for the ring_vector class.
         *
         * This constructor creates a new ring_vector object by copying the contents
         * of another ring_vector object.
         *
         * @param other The ring_vector object to copy from.
         */
        ring_vector(const ring_vector& other)
            : m_ring_vector { other.m_ring_vector }
            , m_push_index { other.m_push_index }
            , m_pop_index { other.m_pop_index }
            , m_last_index { other.m_last_index }
            , m_size { other.m_size }
            , m_capacity { other.m_capacity }
        {
        }

        /**
         * @brief Move constructor for the ring_vector class.
         *
         * This constructor initializes a ring_vector object by transferring ownership
         * of the data from another ring_vector object. The other ring_vector object
         * is left in a valid but unspecified state.
         *
         * @param other The ring_vector object to move from.
         */
        ring_vector(ring_vector&& other) noexcept
            : m_ring_vector { std::move(other.m_ring_vector) }
            , m_push_index { std::move(other.m_push_index) }
            , m_pop_index { std::move(other.m_pop_index) }
            , m_last_index { std::move(other.m_last_index) }
            , m_size { std::move(other.m_size) }
            , m_capacity { std::move(other.m_capacity) }
        {
        }

        /**
         * @brief Assignment operator for the ring_vector class.
         *
         * This operator assigns the contents of another ring_vector instance to this instance.
         * It performs a deep copy of the internal state of the ring_vector.
         *
         * @param other The ring_vector instance to be copied.
         * @return A reference to this ring_vector instance.
         */
        ring_vector& operator=(const ring_vector& other)
        {
            if (this != &other)
            {
                m_ring_vector = other.m_ring_vector;
                m_push_index = other.m_push_index;
                m_pop_index = other.m_pop_index;
                m_last_index = other.m_last_index;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
            }

            return *this;
        }

        /**
         * @brief Move assignment operator for ring_vector.
         *
         * This operator moves the contents of another ring_vector into this one.
         * It transfers ownership of the internal data from the source object to the destination object.
         *
         * @param other The ring_vector to move from.
         * @return A reference to this ring_vector.
         */
        ring_vector& operator=(ring_vector&& other) noexcept
        {
            if (this != &other)
            {
                m_ring_vector = std::move(other.m_ring_vector);
                m_push_index = std::move(other.m_push_index);
                m_pop_index = std::move(other.m_pop_index);
                m_last_index = std::move(other.m_last_index);
                m_size = std::move(other.m_size);
                m_capacity = std::move(other.m_capacity);
            }

            return *this;
        }

        /**
         * @brief Pushes an element into the ring vector.
         *
         * @param elem The element to be pushed into the ring vector.
         */
        void push(const T& elem)
        {
            m_ring_vector[m_push_index] = elem;
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
            if (m_size > m_capacity)
            {
                // first entry is overwritten
                m_pop_index = next_index(m_pop_index);
            }
        }

        /**
         * @brief Inserts an element in place into the ring vector at the current push index.
         *
         * @param elem The element to be inserted into the ring vector.
         */
        void emplace(T&& elem)
        {
            m_ring_vector[m_push_index] = std::move(elem);
            m_last_index = m_push_index;
            m_push_index = next_index(m_push_index);
            ++m_size;
            if (m_size > m_capacity)
            {
                // first entry is overwritten
                m_pop_index = next_index(m_pop_index);
            }
        }

        /**
         * @brief Removes the oldest element from the ring buffer.
         *
         */
        void pop()
        {
            if (!empty())
            {
                m_pop_index = next_index(m_pop_index);
                --m_size;
            }
        }

        /**
         * @brief Returns the front element of the ring vector.
         *
         * This function retrieves the element at the front of the ring vector
         * without removing it.
         *
         * @return T The front element of the ring vector.
         */
        [[nodiscard]] T front() const
        {
            return m_ring_vector[m_pop_index];
        }

        /**
         * @brief Returns the last element in the ring vector.
         *
         * @return The last element of type T in the ring vector.
         */
        [[nodiscard]] T back() const
        {
            return m_ring_vector[m_last_index];
        }

        /**
         * @brief Checks if the ring buffer is empty.
         *
         * This function returns true if the ring buffer is empty, i.e.,
         * the push index is equal to the pop index.
         *
         * @return true if the ring buffer is empty, false otherwise.
         */
        [[nodiscard]] bool empty() const
        {
            return 0U == m_size;
        }

        /**
         * @brief Checks if the ring buffer is full.
         *
         * This method determines if the ring buffer has reached its maximum capacity.
         *
         * @return true if the ring buffer is full, false otherwise.
         */
        [[nodiscard]] bool full() const
        {
            return m_capacity <= m_size;
        }

        /**
         * @brief Clears the ring vector, resetting all indices and size to zero.
         *
         * This function resets the push, pop, and last indices to zero, sets the size to zero,
         * clears the internal ring vector, and resizes it to its capacity.
         */
        void clear()
        {
            m_push_index = 0U;
            m_pop_index = 0U;
            m_last_index = 0U;
            m_size = 0U;
            m_ring_vector.clear();
            m_ring_vector.resize(m_capacity);
        }

        /**
         * @brief Returns the current size of the ring vector.
         *
         * @return std::size_t The number of elements in the ring vector.
         */
        [[nodiscard]] std::size_t size() const
        {
            return m_size;
        }

        /**
         * @brief Returns the capacity of the ring vector.
         *
         * @return std::size_t The capacity of the ring vector.
         */
        [[nodiscard]] std::size_t capacity() const
        {
            return m_capacity;
        }

        // C++17: batch insertion via iterator pair
        /**
         * @brief Inserts a range of elements into the ring vector (C++17).
         *
         * Uses iterator pairs to insert multiple elements. Stops at capacity limit.
         * @tparam InputIt Input iterator type
         * @param first Iterator to first element
         * @param last Iterator to past-the-end element
         * @return Number of elements actually inserted
         */
        template <typename InputIt>
        std::size_t push_range(InputIt first, InputIt last)
        {
            std::size_t inserted = 0;
            for (; first != last && !full(); ++first)
            {
                push(*first);
                ++inserted;
            }
            return inserted;
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: batch insertion via range concept
        /**
         * @brief Inserts a range of elements into the ring vector (C++20).
         *
         * Uses std::ranges concept for container acceptance. Stops at capacity limit.
         * @tparam Range Input range type (requires std::ranges::input_range)
         * @param range The input range to insert from
         * @return Number of elements actually inserted
         */
        template <typename Range>
        requires std::ranges::input_range<Range> && std::is_assignable_v<T&, std::ranges::range_reference_t<Range>>
        std::size_t push_range(Range&& range)
        {
            std::size_t inserted = 0;
            for (auto&& elem : range)
            {
                if (full())
                    break;
                push(std::forward<decltype(elem)>(elem));
                ++inserted;
            }
            return inserted;
        }
#endif

        // C++17: batch extraction via iterator pair
        /**
         * @brief Extracts multiple elements from the ring vector (C++17).
         *
         * Uses iterator pairs to extract elements. Stops when ring is empty or output full.
         * @tparam OutputIt Output iterator type
         * @param first Iterator to first output position
         * @param last Iterator to past-the-end output position
         * @return Number of elements actually extracted
         */
        template <typename OutputIt>
        std::size_t pop_range(OutputIt first, OutputIt last)
        {
            std::size_t popped = 0;
            for (; first != last && !empty(); ++first)
            {
                *first = front();
                pop();
                ++popped;
            }
            return popped;
        }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: batch extraction via span
        /**
         * @brief Extracts multiple elements into a span (C++20).
         *
         * Uses std::span for contiguous output buffer. Returns actual count extracted.
         * @param buffer Output span to fill
         * @return Number of elements actually extracted
         */
        std::size_t pop_range(std::span<T> buffer)
        {
            std::size_t popped = 0;
            for (auto& elem : buffer)
            {
                if (empty())
                    break;
                elem = front();
                pop();
                ++popped;
            }
            return popped;
        }
#endif

        /**
         * @brief Accesses the element at the specified index in the ring vector.
         *
         * @param index The index of the element to access.
         * @return T& Reference to the element at the specified index.
         */
        T& operator[](std::size_t index)
        {
            return m_ring_vector[next_step_index(m_pop_index, index)];
        };

        /**
         * @brief Resizes the ring vector to the specified new capacity.
         *
         * This function resizes the ring vector to the given new capacity. If the new capacity is smaller than the
         * current size, the oldest elements will be discarded. If the new capacity is larger, the ring vector will be
         * expanded to accommodate the new capacity.
         *
         * @param new_capacity The new capacity for the ring vector.
         */
        void resize(std::size_t new_capacity)
        {
            if (m_size == new_capacity)
            {
                return;
            }

            std::vector<T> tmp;
            tmp.reserve(m_size);

            // vector filled from first to last pushed
            std::size_t idx = m_pop_index;
            for (std::size_t i = 0; i < m_size; ++i)
            {
                tmp.emplace_back(std::move(m_ring_vector[idx]));
                idx = next_index(idx);
            }

            m_ring_vector.clear();
            m_ring_vector.resize(new_capacity);

            if (m_size > new_capacity)
            {
                // shrink
                // skip first pushed elements if we resize with a lower capacity
                auto to_skip = (m_size - new_capacity);
                for (std::size_t i = to_skip; i < tmp.size(); ++i)
                {
                    m_ring_vector[i - to_skip] = std::move(tmp[i]);
                }
                m_size -= to_skip;
            }
            else
            {
                for (std::size_t i = 0; i < m_size; ++i)
                {
                    m_ring_vector[i] = std::move(tmp[i]);
                }
            }

            m_capacity = new_capacity;

            m_pop_index = 0U;

            if (m_size > 0U)
            {
                m_push_index = next_step_index(m_pop_index, m_size);
                m_last_index = next_step_index(m_pop_index, m_size - 1U);
            }
            else
            {
                m_push_index = 0U;
                m_last_index = 0U;
            }
        }

    private:
        /**
         * @brief Calculates the next index in a circular buffer.
         *
         * This function computes the next index in a circular buffer based on the current index.
         * It wraps around to the beginning if the end of the buffer is reached.
         *
         * @param index The current index in the buffer.
         * @return The next index in the buffer.
         */
        [[nodiscard]] std::size_t next_index(std::size_t index) const
        {
            return ((index + 1U) % m_capacity);
        }

        /**
         * @brief Calculates the next index in the ring buffer given a current index and a step.
         *
         * This function computes the next index in a circular manner by adding the step to the current index
         * and taking the modulus with the capacity of the ring buffer.
         *
         * @param index The current index in the ring buffer.
         * @param step The number of steps to move forward from the current index.
         * @return The next index in the ring buffer after moving the specified number of steps.
         */
        [[nodiscard]] std::size_t next_step_index(std::size_t index, std::size_t step) const
        {
            return ((index + step) % m_capacity);
        }

        std::vector<T> m_ring_vector = {};
        std::size_t m_push_index = 0U;
        std::size_t m_pop_index = 0U;
        std::size_t m_last_index = 0U;
        std::size_t m_size = 0U;
        std::size_t m_capacity = 0U;
    };
}

#endif //  RING_VECTOR_HPP_
