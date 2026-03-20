/**
 * @file mem_pool_allocator.cpp
 * @brief Custom mem pool allocator (caching/reusing) overriding the global new/delete
 * @author Laurent Lardinois
 * @date March 2026
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

#if defined(USE_MEM_POOL_ALLOCATOR)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <mutex>
#include <new>
#include <numeric>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <bit>
#endif

#include "tools/lock_free_ring_buffer.hpp"

namespace
{
    // Private structure in .bss segment (no heap allocation at all !)

    // https://www.rastergrid.com/blog/sw-eng/2021/03/custom-memory-allocators/

    // We cache and reuse small blocks of memory to prevent memory fragmentation
    // with small frequent events messages.
    //
    // The structure relies on lock free ring buffers and a pair of writer/reader mutexes
    // for the situation where multiple writers or multiple readers request the same size of block
    //
    // The idea is to allocate and cache only blocks with a power of 2 granularity
    // (typically from 16-bytes to 512-bytes or 1024-bytes).
    //
    // Blocks that are greater will not be cached as we make the hypothesis
    // that big chunks of memory will be pre-allocated and reused in dedicated pools
    // if necessary.

    constexpr int MIN_CACHED_BLOCK_POW2_SIZE = 4; // 2^4 = 16 bytes
    constexpr int MAX_CACHED_BLOCK_POW2_SIZE = 9; // 2^9 = 512 bytes

    constexpr std::size_t MIN_CACHED_BLOCK_SIZE = (1U << MIN_CACHED_BLOCK_POW2_SIZE);
    constexpr std::size_t MAX_CACHED_BLOCK_SIZE = (1U << MAX_CACHED_BLOCK_POW2_SIZE);

    constexpr std::size_t MAX_CACHED_BLOCKS_POW2 = 9; // 2^9 = 512 per pool

    struct block_pool
    {
        std::mutex m_push_mtx;
        std::mutex m_pop_mtx;
        tools::lock_free_ring_buffer<void*, MAX_CACHED_BLOCKS_POW2> m_pool;
    };

    using blocks_cache = std::array<block_pool, MAX_CACHED_BLOCK_POW2_SIZE - MIN_CACHED_BLOCK_POW2_SIZE + 1>;

    // data structure statically allocated in .bss region
    blocks_cache g_mem_cache = {};

    // Round up to the next power-of-two with a C++17 fallback.
    constexpr std::size_t bit_ceil_size(std::size_t value)
    {
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        return std::bit_ceil(value);
#else
        if (value <= 1U)
        {
            return 1U;
        }

        --value;
        for (std::size_t shift = 1U; shift < (sizeof(std::size_t) * 8U); shift <<= 1U)
        {
            value |= (value >> shift);
        }
        return value + 1U;
#endif
    }

    // Compute log2(value) for a non-zero power-of-two value.
    constexpr int log2_pow2(std::size_t value)
    {
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        return static_cast<int>(std::countr_zero(value));
#else
        int log2_value = 0;
        while ((value >>= 1U) != 0U)
        {
            ++log2_value;
        }
        return log2_value;
#endif
    }

    constexpr std::size_t pow2_blocksize(std::size_t orig_size)
    {
        return std::max(bit_ceil_size(orig_size), static_cast<std::size_t>(MIN_CACHED_BLOCK_SIZE));
    }

    constexpr int log2int(std::size_t pow2_size)
    {
        // input is non-zero and is a pow of 2 (computed previously with pow2_blocksize)
        return log2_pow2(pow2_size) + 1;
    }

    void* cache_alloc(std::size_t size_pow2)
    {
        // reuse a block if possible
        const int idx = log2int(size_pow2) - MIN_CACHED_BLOCK_POW2_SIZE - 1;

        auto& cache_entry = g_mem_cache[idx];
        void* cached_ptr = nullptr;

        {
            // reader
            std::lock_guard<std::mutex> guard(cache_entry.m_pop_mtx);
            cache_entry.m_pool.pop(cached_ptr);
        }

        // reused block or nullptr
        return cached_ptr;
    }

    bool cache_recycle(void* ptr, std::size_t size_pow2)
    {
        // recycle the block if possible
        const int idx = log2int(size_pow2) - MIN_CACHED_BLOCK_POW2_SIZE - 1;

        auto& cache_entry = g_mem_cache[idx];

        // writer
        std::lock_guard<std::mutex> guard(cache_entry.m_push_mtx);

        return cache_entry.m_pool.push(ptr);
    }

    void* cached_new(std::size_t size)
    {
        const auto size_pow2 = pow2_blocksize(size);

        if (size_pow2 <= MAX_CACHED_BLOCK_SIZE)
        {
            if (void* cached_ptr = cache_alloc(size_pow2))
            {
                // std::cout << "[reuse] " << size_pow2 << " bytes\n";
                return cached_ptr;
            }

            // allocate a pow of 2 block from the heap
            size = size_pow2;
        }

        // fallback - allocate a new block on the heap
        if (void* ptr = std::malloc(size))
        {
            // std::cout << "[alloc] " << size << " bytes\n";
            return ptr;
        }

        throw std::bad_alloc();
    }

    void cached_delete(void* ptr, std::size_t size) noexcept
    {
        // check opportunity to give the released block to the pool
        const auto size_pow2 = pow2_blocksize(size);

        if (size_pow2 <= MAX_CACHED_BLOCK_SIZE)
        {
            const bool recycled = cache_recycle(ptr, size_pow2);

            if (recycled)
            {
                // std::cout << "[recycle] " << size_pow2 << " bytes\n";
                //  block recycled
                return;
            }

            // fallback on regular heap deallocation
        }

        // std::cout << "[free] sized: " << size << " bytes\n";
        std::free(ptr);
    }
}

void init_mem_pool_allocator()
{
#if defined(USE_MEM_POOL_ALLOCATOR_WARMUP)

    // warm up

    std::size_t block_size = MIN_CACHED_BLOCK_SIZE;
    std::size_t total_mem = 0U;

    for (auto& entry : g_mem_cache)
    {
        std::scoped_lock lock(entry.m_pop_mtx, entry.m_push_mtx);

        for (int i = 0; i < (1 << MAX_CACHED_BLOCKS_POW2) - 1; ++i)
        {
            if (void* ptr = std::malloc(block_size))
            {
                // std::cout << "[pre-alloc] " << block_size << " bytes\n";

                total_mem += block_size;

                if (!entry.m_pool.push(ptr))
                {
                    // std::cerr << "[pre-alloc] " << block_size << " bytes failed !\n";
                }
            }
            else
            {
                return;
            }
        }

        block_size <<= 1;
    } // end fill memory cache

    // std::cout << "[total mem] " << total_mem << " bytes pre-allocated in memory pool\n";
#endif
}

void destroy_mem_pool_allocator()
{
    // release all blocks from the pool

    for (auto& entry : g_mem_cache)
    {
        std::scoped_lock lock(entry.m_pop_mtx, entry.m_push_mtx);

        void* block = nullptr;
        while (entry.m_pool.pop(block))
        {
            std::free(block);
        }
    }
}

// ------------------------------------------------------------
//  Global operator new (scalar)
// ------------------------------------------------------------
void* operator new(std::size_t size)
{
    // std::cout << "[new] " << size << " bytes\n";
    return cached_new(size);
}

// ------------------------------------------------------------
//  Global operator delete (scalar, unsized)
// ------------------------------------------------------------
void operator delete(void* ptr) noexcept
{
    // std::cout << "[delete] unsized\n";
    std::free(ptr);
}

// ------------------------------------------------------------
//  Global operator delete (scalar, sized) — C++14+
//  Called when the compiler knows the size at delete time.
// ------------------------------------------------------------
void operator delete(void* ptr, std::size_t size) noexcept
{
    // std::cout << "[delete] sized: " << size << " bytes\n";
    cached_delete(ptr, size);
}

// ------------------------------------------------------------
//  Array versions
// ------------------------------------------------------------
void* operator new[](std::size_t size)
{
    // std::cout << "[new[]] " << size << " bytes\n";
    return cached_new(size);
}

void operator delete[](void* ptr) noexcept
{
    // std::cout << "[delete[]] unsized\n";
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
    // std::cout << "[delete[]] sized: " << size << " bytes\n";
    cached_delete(ptr, size);
}

#if 0
// ---------------------------------------------------------------------
//  Nothrow versions (not cached because there is no sized destructors
// , just logged)
// ---------------------------------------------------------------------
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    // std::cout << "[new nothrow] " << size << " bytes\n";
    return std::malloc(size);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    // std::cout << "[delete nothrow] unsized\n";
    std::free(ptr);
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept
{
    // std::cout << "[new[] nothrow] " << size << " bytes\n";
    return std::malloc(size);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    // std::cout << "[delete[] nothrow] unsized\n";
    std::free(ptr);
}
#endif

// ------------------------------------------------------------
//  Example usage
// ------------------------------------------------------------
/*
int main() {
    int* p = new int(42);
    delete p;

    int* arr = new int[10];
    delete[] arr;

    int* q = new(std::nothrow) int(7);
    delete q;

    return 0;
}
*/

#endif // USE_MEM_POOL_ALLOCATOR
