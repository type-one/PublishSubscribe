/**
 * @file sync_object.hpp
 * @brief Synchronization object using standard C++ constructs.
 *
 * This file contains the definition of the sync_object class, which provides
 * a synchronization mechanism using standard C++ constructs such as mutexes and
 * condition variables.
 *
 * @author Laurent Lardinois
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

#if !defined(SYNC_OBJECT_HPP_)
#define SYNC_OBJECT_HPP_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "tools/non_copyable.hpp"

namespace tools
{
    class sync_object : public non_copyable // NOLINT inherits from non copyable/non movable class
    {
    public:
        sync_object() = default;
        ~sync_object();

        void signal();
        void signal_all();
        void wait_for_signal();
        void wait_for_signal(const std::chrono::duration<int, std::micro>& timeout);

    private:
        bool m_signaled = false;
        std::mutex m_mutex;
        std::condition_variable m_cond;
    };
}

#endif //  SYNC_OBJECT_HPP_
