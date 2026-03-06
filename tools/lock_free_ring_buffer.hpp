/**
 * @file lock_free_ring_buffer.hpp
 * @brief A lock-free ring buffer implementation for single producer and single consumer.
 *
 * This header file contains the implementation of a lock-free ring buffer that supports
 * single producer and single consumer. It ensures thread-safe operations without using locks.
 *
 * @author Laurent Lardinois
 * @date February 2025
 */

//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois      //
//                                                                             //
// https://github.com/type-one/PublishSubscribeESP32                           //
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

#if !defined(LOCK_FREE_RING_BUFFER_HPP_)
#define LOCK_FREE_RING_BUFFER_HPP_

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "tools/non_copyable.hpp"

namespace tools
{
    /**
     * @brief A lock-free ring buffer implementation.
     *
     * This class provides a lock-free ring buffer that supports single producer and single consumer.
     * It ensures that the operations are thread-safe without using locks.
     *
     * @tparam T The type of elements stored in the ring buffer.
     * @tparam Pow2 The power of 2 that determines the size of the ring buffer.
     */
    template <typename T, std::size_t Pow2>
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    requires std::is_standard_layout_v<T> && std::is_trivial_v<T> &&(std::is_scalar_v<T> || std::is_pointer_v<T>)
#endif
        class lock_free_ring_buffer : public non_copyable // NOLINT inherits from non copyable and non movable class
    {
    public:
        static_assert(std::is_standard_layout<T>::value, "T has to provide standard layout");
        static_assert(std::is_trivial<T>::value, "T has to be trivial type");
        static_assert(std::is_scalar<T>::value || std::is_pointer<T>::value, "T has to be scalar or pointer");

        struct thread_safe
        {
            // Single Producer - Single Consumer only
            static constexpr bool value = false;
        };

        lock_free_ring_buffer() = default;
        ~lock_free_ring_buffer() = default;

        /**
         * @brief Pushes an element into the ring buffer.
         *
         * This function attempts to push an element into the ring buffer. If the buffer is full,
         * the function returns false. It handles potential race conditions by checking the indices
         * and waiting if necessary.
         *
         * @param elem The element to be pushed into the ring buffer.
         * @return true if the element was successfully pushed into the buffer, false if the buffer is full.
         */
        bool push(const T& elem)
        {
            const std::size_t snap_write_idx = m_push_index.load();
            const std::size_t snap_read_idx = m_pop_index.load();

            // is full ?
            if ((snap_read_idx & ring_buffer_mask) == ((snap_write_idx + 1U) & ring_buffer_mask))
            {
                return false;
            }

            // getting close or wrap around, risk of race condition
            if (((snap_write_idx - snap_read_idx) <= 2U) || (snap_write_idx < snap_read_idx))
            {
                do
                {
                } while (m_reading.load());
            }

            m_writing.store(true);
            const std::size_t write_idx = m_push_index.fetch_add(1U);
            m_ring_buffer.at(write_idx & ring_buffer_mask).store(elem);
            m_writing.store(false);

            return true;
        }

        /**
         * @brief Pops an element from the ring buffer.
         *
         * This function attempts to pop an element from the ring buffer. If the buffer is empty,
         * the function returns false. If the buffer is not empty, it retrieves the next element
         * and updates the read index.
         *
         * @param elem Reference to the element where the popped value will be stored.
         * @return true if an element was successfully popped, false if the buffer is empty.
         */
        bool pop(T& elem)
        {
            const std::size_t snap_write_idx = m_push_index.load();
            const std::size_t snap_read_idx = m_pop_index.load();

            // is empty ?
            if ((snap_read_idx & ring_buffer_mask) == (snap_write_idx & ring_buffer_mask))
            {
                return false;
            }

            // getting close or wrap around, risk of race condition
            if (((snap_write_idx - snap_read_idx) <= 2U) || (snap_write_idx < snap_read_idx))
            {
                do
                {
                } while (m_writing.load());
            }

            m_reading.store(true);
            const std::size_t read_idx = m_pop_index.fetch_add(1U);
            elem = m_ring_buffer.at(read_idx & ring_buffer_mask).load();
            m_reading.store(false);

            return true;
        }

        /**
         * @brief Returns the capacity of the ring buffer.
         *
         * This function calculates the capacity of the ring buffer based on the
         * template parameter Pow2. The capacity is determined as 2 raised to the
         * power of Pow2.
         *
         * @return The capacity of the ring buffer.
         */
        [[nodiscard]] constexpr std::size_t capacity() const
        {
            return 1U << Pow2;
        }

    private:
        static constexpr const std::size_t ring_buffer_size = (1U << Pow2);
        static constexpr const std::size_t ring_buffer_mask = (ring_buffer_size - 1U);

        std::array<std::atomic<T>, ring_buffer_size> m_ring_buffer;
        std::atomic<std::size_t> m_push_index = 0U;
        std::atomic<std::size_t> m_pop_index = 0U;
        std::atomic_bool m_reading = false;
        std::atomic_bool m_writing = false;
    };
}

#endif //  LOCK_FREE_RING_BUFFER_HPP_
