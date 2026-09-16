/**
 * @file example_common.hpp
 * @brief Demonstrates legacy examples for the Publish/Subscribe pattern.
 *
 * @author Laurent Lardinois
 * @date September 2026
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

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#include <span>
#endif

#include "tools/async_observer.hpp"
#include "tools/expected.hpp"
#include "tools/histogram.hpp"
#include "tools/lock_free_ring_buffer.hpp"
#include "tools/periodic_task.hpp"
#include "tools/ring_buffer.hpp"
#include "tools/ring_vector.hpp"
#include "tools/sync_dictionary.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_priority_queue.hpp"
#include "tools/sync_queue.hpp"
#include "tools/sync_ring_buffer.hpp"
#include "tools/sync_ring_vector.hpp"
#include "tools/sync_time_list.hpp"
#include "tools/time_list.hpp"
#include "tools/worker_task.hpp"

#include "portable_concurrency/p_latch.hpp"
#include "portable_concurrency/p_thread_pool.hpp"
#include "portable_concurrency/p_timed_waiter.hpp"

//--------------------------------------------------------------------------------------------------------------------------------

template <typename T, std::size_t Capacity>
void drain_ring_buffer(tools::ring_buffer<T, Capacity>& queue)
{
    while (!queue.empty())
    {
        std::cout << "  " << queue.front() << std::endl;
        queue.pop();
    }
}

template <typename T, std::size_t Capacity>
void drain_sync_ring_buffer(tools::sync_ring_buffer<T, Capacity>& queue)
{
    while (!queue.empty())
    {
        auto value = queue.front_pop();
        if (value.has_value())
        {
            std::cout << "  " << *value << std::endl;
        }
    }
}

template <typename T>
void drain_ring_vector(tools::ring_vector<T>& vec)
{
    while (!vec.empty())
    {
        std::cout << "  " << vec.front() << std::endl;
        vec.pop();
    }
}

template <typename T>
void drain_sync_ring_vector(tools::sync_ring_vector<T>& vec)
{
    while (!vec.empty())
    {
        auto value = vec.front_pop();
        if (value.has_value())
        {
            std::cout << "  " << *value << std::endl;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
