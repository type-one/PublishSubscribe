/**
 * @file example_allocator_stress.cpp
 * @brief Runs the allocator stress example.
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

#include "example_common.hpp"

namespace
{
    constexpr std::size_t ALLOC_MAX_SIZE = 512;
    constexpr std::size_t ALLOC_ITERATIONS = 10000000;

    enum class alloc_type : std::uint8_t
    {
        new_object,
        new_array
    };

    struct allocation
    {
        void* ptr;
        alloc_type type;
    };

    void alloc_dealloc_worker(int id)
    {
        std::cout << "-- worker " << id << '\n';

        std::mt19937 rng(std::random_device {}());
        std::uniform_int_distribution<std::size_t> size_dist(1, ALLOC_MAX_SIZE);
        std::uniform_int_distribution<int> op_dist(0, 3); // 0=new, 1=new[], 2=delete, 3=delete[]

        std::vector<allocation> allocated;
        allocated.reserve(10000);

        for (std::size_t i = 0; i < ALLOC_ITERATIONS; ++i)
        {
            int oper = op_dist(rng);

            if (oper == 0)
            {
                // new (single object)
                std::size_t ssz = size_dist(rng);
                char* ptr = new char[ssz]; // treat as object
                allocated.push_back({ ptr, alloc_type::new_object });
            }
            else if (oper == 1)
            {
                // new[] (array)
                std::size_t ssz = size_dist(rng);
                char* ptr = new char[ssz];
                allocated.push_back({ ptr, alloc_type::new_array });
            }
            else if (!allocated.empty())
            {
                // delete or delete[]
                std::size_t idx = rng() % allocated.size();
                allocation alloc = allocated[idx];

                if (oper == 2 && alloc.type == alloc_type::new_object)
                {
                    delete static_cast<char*>(alloc.ptr);
                }
                else if (oper == 3 && alloc.type == alloc_type::new_array)
                {
                    delete[] static_cast<char*>(alloc.ptr);
                }
                else
                {
                    // wrong operator chosen → fallback to correct one
                    if (alloc.type == alloc_type::new_object)
                    {
                        delete static_cast<char*>(alloc.ptr);
                    }
                    else
                    {
                        delete[] static_cast<char*>(alloc.ptr);
                    }
                }

                allocated[idx] = allocated.back();

                allocated.pop_back();
            }
        } // cleanup

        for (auto& alloc : allocated)
        {
            if (alloc.type == alloc_type::new_object)
            {
                delete static_cast<char*>(alloc.ptr);
            }
            else
            {
                delete[] static_cast<char*>(alloc.ptr);
            }
        }
    }
}

void test_allocator_stress()
{
    std::cout << "-- allocator stress --\n";

    const auto start = std::chrono::high_resolution_clock::now();
    std::thread thr1(alloc_dealloc_worker, 1);
    std::thread thr2(alloc_dealloc_worker, 2);
    thr1.join();
    thr2.join();
    const auto end = std::chrono::high_resolution_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "allocation/deallocation total time: " << millis << " ms\n";
}
//--------------------------------------------------------------------------------------------------------------------------------


void run_example_allocator_stress()
{
    test_allocator_stress();
}
