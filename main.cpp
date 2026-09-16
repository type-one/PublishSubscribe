/**
 * @file main.cpp
 * @brief Application runner for the PublishSubscribe examples.
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

#include <iostream>

#include "examples/examples.hpp"

#if defined(USE_MEM_POOL_ALLOCATOR)
extern void init_mem_pool_allocator();
extern void destroy_mem_pool_allocator();
#endif

int main()
{
#if defined(USE_MEM_POOL_ALLOCATOR)
    init_mem_pool_allocator();
#endif

    run_example_ring_container();
    run_example_sync_container();
    run_example_time_list();
    run_example_pub_sub_and_task();
    run_example_worker_and_command();
    run_example_data_task();
    run_example_allocator_stress();

#if defined(USE_MEM_POOL_ALLOCATOR)
    destroy_mem_pool_allocator();
#endif

    std::cout << "This is The END" << '\n';
    return 0;
}
