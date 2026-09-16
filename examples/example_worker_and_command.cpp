/**
 * @file example_worker_and_command.cpp
 * @brief Runs queued command and worker task examples.
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

#include "examples.hpp"

void test_queued_commands();
void test_ring_buffer_commands();
void test_worker_tasks();
void test_worker_tasks_async();
void test_worker_tasks_async_fanout();
void test_portable_concurrency_test_parity();
#if defined(PC_HAS_COROUTINES)
void test_worker_tasks_coroutine_schedule();
void test_worker_tasks_mixed_execution();
#endif

void run_example_worker_and_command()
{
    test_queued_commands();
    test_ring_buffer_commands();
    test_worker_tasks();
    test_worker_tasks_async();
    test_worker_tasks_async_fanout();
    test_portable_concurrency_test_parity();
#if defined(PC_HAS_COROUTINES)
    test_worker_tasks_coroutine_schedule();
    test_worker_tasks_mixed_execution();
#endif
}
