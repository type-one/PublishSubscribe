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

#if !defined(__PERIODIC_TASK_HPP__)
#define __PERIODIC_TASK_HPP__

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "non_copyable.hpp"

namespace tools
{
    template<typename Context>
    class periodic_task : public non_copyable
    {

    public:    
        periodic_task() = delete;        

        periodic_task(std::function<void(std::shared_ptr<Context>)>&& routine,
                      std::shared_ptr<Context> context,
                      const std::chrono::duration<int, std::micro>& period)
            : m_routine(std::move(routine))
            , m_context(context)
            , m_period(period)
        {
            m_task = std::make_unique<std::thread>([this]()
            {
                periodic_call();
            });
        }

        ~periodic_task()
        {
            m_stop_task.store(true);
            m_task->join();
        }

    private:

        void periodic_call()
        {
            auto start_time = std::chrono::high_resolution_clock::now();
            auto deadline = start_time + m_period;

            while(!m_stop_task.load())
            {   
                m_routine(m_context);                

                auto current_time = std::chrono::high_resolution_clock::now();

                if (deadline > current_time)
                { 
                    // sleep until we are close to the deadline (at 90%)
                    const double ratio = 0.9;
                    auto remaining_time = std::chrono::duration_cast<std::chrono::microseconds>(deadline - current_time);
                    const auto sleep_time = std::chrono::duration<int, std::micro>(static_cast<int>(ratio*remaining_time.count()));
                    std::this_thread::sleep_for(sleep_time);

                    current_time = std::chrono::high_resolution_clock::now();
                    
                    // active wait loop
                    while (deadline > current_time)
                    {
                        current_time = std::chrono::high_resolution_clock::now();
                    }
                }  

                deadline += m_period;              
            }
        }      

        std::function<void(std::shared_ptr<Context>)> m_routine;
        std::shared_ptr<Context> m_context;
        std::chrono::duration<int, std::micro> m_period;
        std::unique_ptr<std::thread> m_task;
        std::atomic_bool m_stop_task = false;
    };
}

#endif //  __PERIODIC_TASK_HPP__
