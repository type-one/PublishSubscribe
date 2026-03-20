/**
 * @file sync_ring_vector.hpp
 * @brief A thread-safe ring vector implementation.
 *
 * This file contains the definition of the sync_ring_vector class, which provides
 * a thread-safe ring vector that supports various operations such as push, pop,
 * front, back, and size. It ensures thread safety using mutexes.
 *
 * @author Laurent Lardinois
 *
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

#if !defined(SYNC_RING_VECTOR_HPP_)
#define SYNC_RING_VECTOR_HPP_

#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <utility>

#include "tools/non_copyable.hpp"
#include "tools/ring_vector.hpp"

namespace tools
{
    /**
     * @brief A thread-safe ring vector implementation.
     *
     * This class provides a thread-safe ring vector that supports various operations
     * such as push, pop, front, back, and size. It ensures thread safety using mutexes.
     *
     * @tparam T The type of elements stored in the ring vector.
     */
    template <typename T>
    class sync_ring_vector : public non_copyable // NOLINT inherits from non copyable and non movable class
    {
    public:
        struct thread_safe
        {
            static constexpr bool value = true;
        };

        sync_ring_vector() = delete;
        ~sync_ring_vector() = default;

        /**
         * @brief Constructs a sync_ring_vector with the specified capacity.
         *
         * @param capacity The maximum number of elements the ring vector can hold.
         */
        explicit sync_ring_vector(std::size_t capacity)
            : m_ring_vector(capacity)
        {
        }

        /**
         * @brief Pushes an element into the ring vector.
         *
         * This method adds an element to the ring vector in a thread-safe manner.
         *
         * @param elem The element to be pushed into the ring vector.
         */
        void push(const T& elem)
        {
            std::unique_lock guard(m_mutex);
            m_ring_vector.push(elem);
        }

        /**
         * @brief Emplaces an element into the ring vector.
         *
         * This function locks the mutex to ensure thread safety and then
         * emplaces the given element into the ring vector.
         *
         * @param elem The element to be emplaced into the ring vector.
         */
        void emplace(T&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_ring_vector.emplace(std::move(elem));
        }

        /**
         * @brief Removes the last element from the ring vector.
         */
        void pop()
        {
            std::unique_lock guard(m_mutex);
            if (!m_ring_vector.empty())
            {
                m_ring_vector.pop();
            }
        }

        /**
         * @brief Retrieves the first element from the ring vector.
         *
         * This method locks the mutex to ensure thread safety and returns the first element
         * from the ring vector.
         *
         * @return The first element of type T from the ring vector, or none if the vector is empty.
         */
        std::optional<T> front() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_ring_vector.empty())
            {
                item = m_ring_vector.front();
            }
            return item;
        }

        /**
         * @brief Retrieves and removes the first element from the ring vector.
         *
         * This method locks the mutex to ensure thread safety and returns and removes the first element
         * from the ring vector.
         *
         * @return The first element of type T from the ring vector, or none if the vector is empty.
         */
        std::optional<T> front_pop()
        {
            std::optional<T> item;
            std::unique_lock guard(m_mutex);
            if (!m_ring_vector.empty())
            {
                item = m_ring_vector.front();
                m_ring_vector.pop();
            }
            return item;
        }

        /**
         * @brief Retrieves the last element from the ring vector.
         *
         * This method locks the mutex to ensure thread safety and returns the last element
         * in the ring vector.
         *
         * @return The last element of type T in the ring vector, or none if the vector is empty.
         */
        std::optional<T> back() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_ring_vector.empty())
            {
                item = m_ring_vector.back();
            }
            return item;
        }

        /**
         * @brief Retrieves a copy of the internal ring vector.
         *
         * This method locks the mutex to ensure thread safety and returns a copy
         * of the internal ring vector.
         *
         * @return A copy of the internal ring vector.
         */
        tools::ring_vector<T> snapshot() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_vector;
        }

        /**
         * @brief Checks if the ring vector is empty.
         *
         * This method acquires a lock on the mutex to ensure thread safety
         * while checking if the ring vector is empty.
         *
         * @return true if the ring vector is empty, false otherwise.
         */
        [[nodiscard]] bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_vector.empty();
        }

        /**
         * @brief Checks if the ring vector is full.
         *
         * This method acquires a lock on the mutex to ensure thread safety
         * and then checks if the ring vector is full.
         *
         * @return true if the ring vector is full, false otherwise.
         */
        [[nodiscard]] bool full() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_vector.full();
        }

        /**
         * @brief Returns the number of elements in the ring vector.
         *
         * This method acquires a lock to ensure thread safety while accessing the size of the ring vector.
         *
         * @return The number of elements in the ring vector.
         */
        [[nodiscard]] std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_vector.size();
        }

        /**
         * @brief Returns the capacity of the ring vector.
         *
         * @return The capacity of the ring vector.
         */
        [[nodiscard]] std::size_t capacity() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_vector.capacity();
        }

        /**
         * @brief Resizes the ring vector to the specified new size.
         *
         * This function changes the size of the ring vector to the new size specified by the parameter.
         * It uses a mutex to ensure thread safety during the resizing operation.
         *
         * @param new_size The new size to which the ring vector should be resized.
         */
        void resize(std::size_t new_size)
        {
            std::unique_lock guard(m_mutex);
            if (new_size != m_ring_vector.size())
            {
                m_ring_vector.resize(new_size);
            }
        }

    private:
        ring_vector<T> m_ring_vector;
        mutable std::shared_mutex m_mutex;
    };
}

#endif //  SYNC_RING_VECTOR_HPP_
