/**
 * @file example_data_task.cpp
 * @brief Demonstrates bounded asynchronous data processing.
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

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "examples.hpp"
#include "tools/data_task.hpp"

namespace
{
    struct data_task_context
    {
        std::atomic_int processed_count = 0;
    };
}

void run_example_data_task()
{
    std::cout << "-- data task --" << '\n';

    auto context = std::make_shared<data_task_context>();
    tools::data_task<data_task_context, int> task(
        [](const std::shared_ptr<data_task_context>&, const std::string& task_name)
        { std::cout << "starting " << task_name << '\n'; },
        [](const std::shared_ptr<data_task_context>& task_context, const int& data, const std::string& task_name)
        {
            std::cout << task_name << " processed " << data << '\n';
            task_context->processed_count.fetch_add(1);
        },
        context, 4U, "data-task-example");

    for (const auto data : { 10, 20, 30 })
    {
        if (!task.submit(data))
        {
            std::cout << "data queue is full; dropped " << data << '\n';
        }
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while (context->processed_count.load() != 3 && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    std::cout << "processed entries: " << context->processed_count.load() << '\n';
}
