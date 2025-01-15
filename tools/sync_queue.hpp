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

#if !defined(__SYNC_QUEUE_HPP__)
#define __SYNC_QUEUE_HPP__

#include <mutex>
#include <queue>

#include "non_copyable.hpp"

namespace tools
{
    template <typename T>
    class sync_queue : public non_copyable
    {
    public:
        sync_queue() = default;
        ~sync_queue() = default;

        void push(const T& elem)
        {
            std::lock_guard guard(m_mutex);
            m_queue.push(elem);
        }

        void emplace(T&& elem)
        {
            std::lock_guard guard(m_mutex);
            m_queue.emplace(elem);
        }

        void pop()
        {
            std::lock_guard guard(m_mutex);
            m_queue.pop();
        }

        T front()
        {
            std::lock_guard guard(m_mutex);
            return m_queue.front();
        }

        T back()
        {
            std::lock_guard guard(m_mutex);
            return m_queue.back();
        }

        bool empty()
        {
            std::lock_guard guard(m_mutex);
            return m_queue.empty();
        }

        std::size_t size()
        {
            std::lock_guard guard(m_mutex);
            return m_queue.size();
        }

    private:
        std::queue<T> m_queue;
        std::mutex m_mutex;
    };
}

#endif //  __SYNC_QUEUE_HPP__
