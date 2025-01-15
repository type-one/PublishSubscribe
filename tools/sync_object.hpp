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

#if !defined(__SYNC_OBJECT_HPP__)
#define __SYNC_OBJECT_HPP__

#include "non_copyable.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace tools
{
    class sync_object : public non_copyable
    {
    public:
        sync_object(bool initial_state = false);
        ~sync_object();

        void signal();
        void wait_for_signal();
        void wait_for_signal(const std::chrono::duration<int, std::micro>& timeout);

    private:
        bool m_signaled;
        bool m_stop;
        std::mutex m_mutex;
        std::condition_variable m_cond;
    };
}

#endif //  __SYNC_OBJECT_HPP__
