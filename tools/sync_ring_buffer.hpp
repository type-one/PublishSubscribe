/**
 * @file sync_ring_buffer.hpp
 * @brief A thread-safe ring buffer implementation.
 *
 * This file contains the definition of a thread-safe ring buffer class template.
 * The ring buffer provides basic operations such as push, pop, front, front_pop,
 * back, and size, and ensures thread safety using a mutex.
 *
 * @author Laurent Lardinois
 *
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

#if !defined(SYNC_RING_BUFFER_HPP_)
#define SYNC_RING_BUFFER_HPP_

#include <cstddef>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <utility>

#include "tools/non_copyable.hpp"
#include "tools/ring_buffer.hpp"

namespace tools
{
    /**
     * @brief A thread-safe ring buffer implementation.
     *
     * This class provides a thread-safe ring buffer with a fixed capacity.
     * It supports basic operations such as push, pop, front, front_pop, back, and size,
     * and ensures thread safety using a mutex.
     *
     * @tparam T The type of elements stored in the ring buffer.
     * @tparam Capacity The maximum number of elements the ring buffer can hold.
     */
    template <typename T, std::size_t Capacity>
    class sync_ring_buffer : public non_copyable // NOLINT inherits from non copyable/non movable
    {
    public:
        sync_ring_buffer() = default;
        ~sync_ring_buffer() = default;

        void push(const T& elem)
        {
            std::unique_lock guard(m_mutex);
            m_ring_buffer.push(elem);
        }

        // rvalue overload: moves an already-constructed element into the buffer
        void push(T&& elem)
        {
            std::unique_lock guard(m_mutex);
            m_ring_buffer.push(std::move(elem));
        }

        // perfect forwarding: constructs T in-place from arbitrary constructor arguments
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains the template to valid T constructors
        template <typename... Args>
            requires std::is_constructible_v<T, Args...>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_ring_buffer.emplace(std::forward<Args>(args)...);
        }
#else
        // C++17: std::enable_if_t provides equivalent SFINAE constraint
        template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        void emplace(Args&&... args)
        {
            std::unique_lock guard(m_mutex);
            m_ring_buffer.emplace(std::forward<Args>(args)...);
        }
#endif

        void pop()
        {
            std::unique_lock guard(m_mutex);
            if (!m_ring_buffer.empty())
            {
                m_ring_buffer.pop();
            }
        }

        std::optional<T> front_pop()
        {
            std::optional<T> item;
            std::unique_lock guard(m_mutex);
            if (!m_ring_buffer.empty())
            {
                item = m_ring_buffer.front();
                m_ring_buffer.pop();
            }
            return item;
        }

        std::optional<T> front() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_ring_buffer.empty())
            {
                item = m_ring_buffer.front();
            }
            return item;
        }

        std::optional<T> back() const
        {
            std::optional<T> item;
            std::shared_lock guard(m_mutex);
            if (!m_ring_buffer.empty())
            {
                item = m_ring_buffer.back();
            }
            return item;
        }

        [[nodiscard]] bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_buffer.empty();
        }

        [[nodiscard]] bool full() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_buffer.full();
        }

        [[nodiscard]] std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_ring_buffer.size();
        }

        [[nodiscard]] constexpr std::size_t capacity() const
        {
            return m_ring_buffer.capacity();
        }

    private:
        ring_buffer<T, Capacity> m_ring_buffer;
        std::shared_mutex m_mutex;
    };
}

#endif // SYNC_RING_BUFFER_HPP_
