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

#if !defined(__SYNC_RING_BUFFER_HPP__)
#define __SYNC_RING_BUFFER_HPP__

#include <cstddef>
#include <mutex>

#include "tools/non_copyable.hpp"
#include "tools/ring_buffer.hpp"

namespace tools
{
    template <typename T, std::size_t Capacity>
    class sync_ring_buffer : public non_copyable
    {
    public:
        sync_ring_buffer() = default;
        ~sync_ring_buffer() = default;

        void push(const T& elem)
        {
            std::lock_guard guard(m_mutex);
            m_ring_buffer.push(elem);
        }

        void emplace(T&& elem)
        {
            std::lock_guard guard(m_mutex);
            m_ring_buffer.emplace(std::move(elem));
        }

        void pop()
        {
            std::lock_guard guard(m_mutex);
            m_ring_buffer.pop();
        }

        T front()
        {
            std::lock_guard guard(m_mutex);
            return m_ring_buffer.front();
        }

        T back()
        {
            std::lock_guard guard(m_mutex);
            return m_ring_buffer.back();
        }

        bool empty()
        {
            std::lock_guard guard(m_mutex);
            return m_ring_buffer.empty();
        }

        bool full()
        {
            std::lock_guard guard(m_mutex);
            return m_ring_buffer.full();
        }

        std::size_t size()
        {
            std::lock_guard guard(m_mutex);
            return m_ring_buffer.size();
        }

        constexpr std::size_t capacity() const { return m_ring_buffer.capacity(); }

    private:
        ring_buffer<T, Capacity> m_ring_buffer;
        std::mutex m_mutex;
    };
}

#endif //  __SYNC_RING_BUFFER_HPP__
