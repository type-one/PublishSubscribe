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

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "tools/sync_object.hpp"

namespace tools
{
    sync_object::sync_object(bool initial_state)
        : m_signaled(initial_state)
        , m_stop(false)
    {
    }

    sync_object::~sync_object()
    {
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_signaled = true;
            m_stop = true;
        }
        m_cond.notify_all();
    }

    void sync_object::signal()
    {
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_signaled = true;
        }
        m_cond.notify_one();
    }

    void sync_object::wait_for_signal()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [&]() { return m_signaled; });
        m_signaled = m_stop;
    }

    void sync_object::wait_for_signal(const std::chrono::duration<int, std::micro>& timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait_for(lock, timeout, [&]() { return m_signaled; });
        m_signaled = m_stop;
    }
}
