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

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#include <span>
#endif

#include "tools/async_observer.hpp"
#include "tools/histogram.hpp"
#include "tools/lock_free_ring_buffer.hpp"
#include "tools/periodic_task.hpp"
#include "tools/ring_buffer.hpp"
#include "tools/ring_vector.hpp"
#include "tools/sync_dictionary.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_queue.hpp"
#include "tools/sync_ring_buffer.hpp"
#include "tools/sync_ring_vector.hpp"
#include "tools/worker_task.hpp"

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

void test_ring_buffer()
{
    std::cout << "-- ring buffer --" << std::endl;
    tools::ring_buffer<std::string, 64U> str_queue;

    // emplace: construct string directly in the buffer
    str_queue.emplace("toto");

    // push rvalue: move a pre-constructed string into the buffer
    std::string moved = "titi";
    str_queue.push(std::move(moved));

    auto item = str_queue.front();

    std::cout << "front after emplace/push: " << item << std::endl;

    std::cout << "drain initial content:" << std::endl;
    drain_ring_buffer(str_queue);

    // push_range (C++17): iterator-pair insertion
    std::vector<std::string> batch = { "alpha", "beta", "gamma" };
    const auto inserted_pair = str_queue.push_range(batch.begin(), batch.end());
    std::cout << "inserted with iterator-pair: " << inserted_pair << std::endl;

    // pop_range (C++17): iterator-pair batch extraction
    std::array<std::string, 2> popped_pair {};
    const auto popped_pair_count = str_queue.pop_range(popped_pair.begin(), popped_pair.end());
    std::cout << "popped with iterator-pair: " << popped_pair_count << std::endl;
    for (std::size_t i = 0; i < popped_pair_count; ++i)
    {
        std::cout << "  " << popped_pair[i] << std::endl;
    }

    std::cout << "drain iterator-pair batch:" << std::endl;
    drain_ring_buffer(str_queue);

    // capacity-bound behavior: insertion stops when full
    tools::ring_buffer<std::string, 4U> small_queue;
    std::vector<std::string> overflow_batch = { "A", "B", "C", "D", "E" };
    const auto inserted_limited = small_queue.push_range(overflow_batch.begin(), overflow_batch.end());
    std::cout << "inserted in capacity-limited buffer: " << inserted_limited << " / " << overflow_batch.size()
              << std::endl;
    std::cout << "small queue full: " << std::boolalpha << small_queue.full() << std::noboolalpha << std::endl;

    std::cout << "drain capacity-limited buffer:" << std::endl;
    drain_ring_buffer(small_queue);

    // reject-on-full mode (single push)
    tools::ring_buffer<std::string, 4U> reject_queue;
    reject_queue.push("R1");
    reject_queue.push("R2");
    reject_queue.push("R3");
    const bool reject_single_ok = reject_queue.push("R4");
    std::cout << "reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << std::endl;
    std::cout << "reject mode contents:" << std::endl;
    drain_ring_buffer(reject_queue);

    // overwrite-on-full mode (single push)
    tools::ring_buffer<std::string, 4U> overwrite_queue;
    overwrite_queue.push("O1");
    overwrite_queue.push("O2");
    overwrite_queue.push("O3");
    const bool overwrite_single_happened = overwrite_queue.push_overwrite("O4");
    std::cout << "overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << std::endl;
    std::cout << "overwrite mode contents (recent history):" << std::endl;
    drain_ring_buffer(overwrite_queue);

    // overwrite-on-full mode (push_range)
    tools::ring_buffer<std::string, 4U> overwrite_range_queue;
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_queue.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << std::endl;
    std::cout << "overwrite range contents (recent history):" << std::endl;
    drain_ring_buffer(overwrite_range_queue);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a container
    std::vector<std::string> range_batch = { "one", "two", "three" };
    const auto inserted_range = str_queue.push_range(range_batch);
    std::cout << "inserted with C++20 range: " << inserted_range << std::endl;

    // pop_range (C++20): span-based batch extraction
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with C++20 span: " << popped_span_count << std::endl;
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  " << popped_span[i] << std::endl;
    }

    std::cout << "drain C++20 container range:" << std::endl;
    drain_ring_buffer(str_queue);

    // push_range (C++20): range overload with a filtered view
    std::vector<std::string> mixed = { "keep_1", "skip", "keep_2", "no" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    const auto inserted_view = str_queue.push_range(filtered);
    std::cout << "inserted with C++20 filtered view: " << inserted_view << std::endl;

    std::cout << "drain C++20 filtered view:" << std::endl;
    drain_ring_buffer(str_queue);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_lock_free_ring_buffer()
{
    std::cout << "-- lock free ring buffer --" << std::endl;
    tools::lock_free_ring_buffer<int, 4U> queue;

    // push_range (C++17): iterator-pair insertion
    std::vector<int> input = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const auto inserted = queue.push_range(input.begin(), input.end());
    std::cout << "push_range inserted: " << inserted << std::endl;

    // pop_range (C++17): iterator-pair extraction
    std::array<int, 6> popped_first {};
    const auto popped_count_first = queue.pop_range(popped_first.begin(), popped_first.end());
    std::cout << "pop_range iterator-pair popped: " << popped_count_first << std::endl;
    for (std::size_t i = 0; i < popped_count_first; ++i)
    {
        std::cout << "  " << popped_first[i] << std::endl;
    }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a filtered view
    std::vector<int> more = { 11, 12, 13, 14, 15, 16, 17, 18 };
    auto evens = more | std::views::filter([](int value) { return (value % 2) == 0; });
    const auto inserted_view = queue.push_range(evens);
    std::cout << "push_range C++20 filtered range inserted: " << inserted_view << std::endl;

    // pop_range (C++20): span overload
    std::array<int, 8> popped_second {};
    const auto popped_count_second = queue.pop_range(std::span<int>(popped_second));
    std::cout << "pop_range C++20 span popped: " << popped_count_second << std::endl;
    for (std::size_t i = 0; i < popped_count_second; ++i)
    {
        std::cout << "  " << popped_second[i] << std::endl;
    }
#endif

    // small SPSC stress test: force wraparound repeatedly and verify FIFO ordering.
    tools::lock_free_ring_buffer<int, 3U> stress_queue;
    constexpr int stress_item_count = 50000;
    std::atomic_bool ordering_ok = true;

    std::thread producer(
        [&stress_queue]()
        {
            for (int value = 0; value < stress_item_count; ++value)
            {
                while (!stress_queue.push(value))
                {
                    std::this_thread::yield();
                }
            }
        });

    std::thread consumer(
        [&stress_queue, &ordering_ok]()
        {
            for (int expected = 0; expected < stress_item_count; ++expected)
            {
                int value = 0;
                while (!stress_queue.pop(value))
                {
                    std::this_thread::yield();
                }

                if (value != expected)
                {
                    ordering_ok.store(false);
                }
            }
        });

    producer.join();
    consumer.join();

    std::cout << "SPSC wraparound stress ordering OK: " << std::boolalpha << ordering_ok.load() << std::noboolalpha
              << std::endl;
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_ring_buffer()
{
    std::cout << "-- sync ring buffer --" << std::endl;
    tools::sync_ring_buffer<std::string, 64U> str_queue;

    // emplace + push(rvalue)
    str_queue.emplace("toto");
    std::string moved = "titi";
    str_queue.push(std::move(moved));

    std::cout << "drain initial sync content:" << std::endl;
    drain_sync_ring_buffer(str_queue);

    // push_range (C++17): iterator-pair insertion under one lock
    std::vector<std::string> batch = { "alpha", "beta", "gamma" };
    const auto inserted_pair = str_queue.push_range(batch.begin(), batch.end());
    std::cout << "inserted with iterator-pair: " << inserted_pair << std::endl;

    // pop_range (C++17): iterator-pair extraction under one lock
    std::array<std::string, 2> popped_pair {};
    const auto popped_pair_count = str_queue.pop_range(popped_pair.begin(), popped_pair.end());
    std::cout << "popped with iterator-pair: " << popped_pair_count << std::endl;
    for (std::size_t i = 0; i < popped_pair_count; ++i)
    {
        std::cout << "  " << popped_pair[i] << std::endl;
    }

    std::cout << "drain iterator-pair sync batch:" << std::endl;
    drain_sync_ring_buffer(str_queue);

    // capacity-bound behavior: insertion stops when full
    tools::sync_ring_buffer<std::string, 4U> small_queue;
    std::vector<std::string> overflow_batch = { "A", "B", "C", "D", "E" };
    const auto inserted_limited = small_queue.push_range(overflow_batch.begin(), overflow_batch.end());
    std::cout << "inserted in capacity-limited sync buffer: " << inserted_limited << " / " << overflow_batch.size()
              << std::endl;
    std::cout << "small sync queue full: " << std::boolalpha << small_queue.full() << std::noboolalpha << std::endl;

    std::cout << "drain capacity-limited sync buffer:" << std::endl;
    drain_sync_ring_buffer(small_queue);

    // reject-on-full mode (single push)
    tools::sync_ring_buffer<std::string, 4U> reject_queue;
    reject_queue.push("R1");
    reject_queue.push("R2");
    reject_queue.push("R3");
    const bool reject_single_ok = reject_queue.push("R4");
    std::cout << "sync reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << std::endl;
    std::cout << "sync reject mode contents:" << std::endl;
    drain_sync_ring_buffer(reject_queue);

    // overwrite-on-full mode (single push)
    tools::sync_ring_buffer<std::string, 4U> overwrite_queue;
    overwrite_queue.push("O1");
    overwrite_queue.push("O2");
    overwrite_queue.push("O3");
    const bool overwrite_single_happened = overwrite_queue.push_overwrite("O4");
    std::cout << "sync overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << std::endl;
    std::cout << "sync overwrite mode contents (recent history):" << std::endl;
    drain_sync_ring_buffer(overwrite_queue);

    // overwrite-on-full mode (push_range)
    tools::sync_ring_buffer<std::string, 4U> overwrite_range_queue;
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_queue.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "sync overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << std::endl;
    std::cout << "sync overwrite range contents (recent history):" << std::endl;
    drain_sync_ring_buffer(overwrite_range_queue);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a container
    std::vector<std::string> range_batch = { "one", "two", "three" };
    const auto inserted_range = str_queue.push_range(range_batch);
    std::cout << "inserted with C++20 range: " << inserted_range << std::endl;

    // pop_range (C++20): span-based extraction under one lock
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with C++20 span: " << popped_span_count << std::endl;
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  " << popped_span[i] << std::endl;
    }

    std::cout << "drain C++20 container sync range:" << std::endl;
    drain_sync_ring_buffer(str_queue);

    // push_range (C++20): range overload with a filtered view
    std::vector<std::string> mixed = { "keep_1", "skip", "keep_2", "no" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    const auto inserted_view = str_queue.push_range(filtered);
    std::cout << "inserted with C++20 filtered view: " << inserted_view << std::endl;

    std::cout << "drain C++20 filtered sync view:" << std::endl;
    drain_sync_ring_buffer(str_queue);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_ring_vector()
{
    std::cout << "-- ring vector --" << std::endl;
    tools::ring_vector<std::string> str_vec(10U);

    // emplace: construct string directly in the vector
    str_vec.emplace("alpha");

    // push rvalue: move a pre-constructed string into the vector
    std::string moved = "beta";
    str_vec.push(std::move(moved));

    auto item = str_vec.front();
    std::cout << "front after emplace/push: " << item << std::endl;

    std::cout << "drain initial content:" << std::endl;
    drain_ring_vector(str_vec);

    // C++17: push_range via iterator-pair insertion
    {
        std::vector<std::string> batch = { "apple", "banana", "cherry" };
        const auto inserted = str_vec.push_range(batch.begin(), batch.end());
        std::cout << "inserted with iterator-pair: " << inserted << std::endl;
    }

    // C++17: pop_range via iterator-pair extraction
    {
        std::array<std::string, 2> output {};
        const auto popped = str_vec.pop_range(output.begin(), output.end());
        std::cout << "popped with iterator-pair: " << popped << std::endl;
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << output[i] << std::endl;
        }
    }

    std::cout << "drain iterator-pair batch:" << std::endl;
    drain_ring_vector(str_vec);

    // reject-on-full mode (single push)
    tools::ring_vector<std::string> reject_vec(4U);
    reject_vec.push("R1");
    reject_vec.push("R2");
    reject_vec.push("R3");
    reject_vec.push("R4");
    const bool reject_single_ok = reject_vec.push("R5");
    std::cout << "reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << std::endl;
    std::cout << "reject mode contents:" << std::endl;
    drain_ring_vector(reject_vec);

    // overwrite-on-full mode (single push)
    tools::ring_vector<std::string> overwrite_vec(4U);
    overwrite_vec.push("O1");
    overwrite_vec.push("O2");
    overwrite_vec.push("O3");
    overwrite_vec.push("O4");
    const bool overwrite_single_happened = overwrite_vec.push_overwrite("O5");
    std::cout << "overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << std::endl;
    std::cout << "overwrite mode contents (recent history):" << std::endl;
    drain_ring_vector(overwrite_vec);

    // overwrite-on-full mode (push_range)
    tools::ring_vector<std::string> overwrite_range_vec(4U);
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result = overwrite_range_vec.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << std::endl;
    std::cout << "overwrite range contents (recent history):" << std::endl;
    drain_ring_vector(overwrite_range_vec);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // C++20: push_range via ranges concept
    {
        std::vector<std::string> batch = { "dog", "elephant", "fox", "giraffe" };
        auto filtered = batch | std::views::filter([](const auto& s) { return s.length() > 3; });
        const auto inserted = str_vec.push_range(filtered);
        std::cout << "inserted with C++20 filtered range: " << inserted << std::endl;
    }

    // C++20: pop_range via span
    {
        std::array<std::string, 3> buffer {};
        const auto popped = str_vec.pop_range(std::span(buffer));
        std::cout << "popped with C++20 span: " << popped << std::endl;
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << buffer[i] << std::endl;
        }
    }

    std::cout << "drain C++20 span batch:" << std::endl;
    drain_ring_vector(str_vec);
#endif

    // Resize test: expand capacity and add more items
    {
        std::cout << "resize test:" << std::endl;
        str_vec.emplace("item1");
        str_vec.emplace("item2");
        str_vec.emplace("item3");
        std::cout << "before resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << std::endl;
        str_vec.resize(20U);
        std::cout << "after expand resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << std::endl;

        // Add more items after resize
        std::vector<std::string> more = { "new1", "new2", "new3", "new4" };
        str_vec.push_range(more.begin(), more.end());
        std::cout << "after push_range: size=" << str_vec.size() << std::endl;

        // Shrink and check if oldest items are dropped
        std::cout << "contents before shrink:" << std::endl;
        for (std::size_t i = 0; i < str_vec.size(); ++i)
        {
            std::cout << "  [" << i << "] " << str_vec[i] << std::endl;
        }

        str_vec.resize(4U);
        std::cout << "after shrink resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << std::endl;
        std::cout << "contents after shrink:" << std::endl;
        for (std::size_t i = 0; i < str_vec.size(); ++i)
        {
            std::cout << "  [" << i << "] " << str_vec[i] << std::endl;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_ring_vector()
{
    std::cout << "-- sync ring vector --" << std::endl;
    tools::sync_ring_vector<std::string> str_vec(10U);

    // emplace: construct string directly in the vector
    str_vec.emplace("initial");

    auto item = str_vec.front();
    if (item.has_value())
    {
        std::cout << "front after emplace: " << *item << std::endl;
    }

    // drain initial content
    std::cout << "drain initial content:" << std::endl;
    drain_sync_ring_vector(str_vec);

    // C++17: push_range via iterator-pair
    {
        std::vector<std::string> batch = { "one", "two", "three" };
        const auto inserted = str_vec.push_range(batch.begin(), batch.end());
        std::cout << "inserted with iterator-pair: " << inserted << std::endl;
    }

    // C++17: pop_range via iterator-pair
    {
        std::array<std::string, 2> output {};
        const auto popped = str_vec.pop_range(output.begin(), output.end());
        std::cout << "popped with iterator-pair: " << popped << std::endl;
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << output[i] << std::endl;
        }
    }

    std::cout << "drain iterator-pair batch:" << std::endl;
    drain_sync_ring_vector(str_vec);

    // reject-on-full mode (single push)
    tools::sync_ring_vector<std::string> reject_vec(4U);
    reject_vec.push("R1");
    reject_vec.push("R2");
    reject_vec.push("R3");
    reject_vec.push("R4");
    const bool reject_single_ok = reject_vec.push("R5");
    std::cout << "sync reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << std::endl;
    std::cout << "sync reject mode contents:" << std::endl;
    drain_sync_ring_vector(reject_vec);

    // overwrite-on-full mode (single push)
    tools::sync_ring_vector<std::string> overwrite_vec(4U);
    overwrite_vec.push("O1");
    overwrite_vec.push("O2");
    overwrite_vec.push("O3");
    overwrite_vec.push("O4");
    const bool overwrite_single_happened = overwrite_vec.push_overwrite("O5");
    std::cout << "sync overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << std::endl;
    std::cout << "sync overwrite mode contents (recent history):" << std::endl;
    drain_sync_ring_vector(overwrite_vec);

    // overwrite-on-full mode (push_range)
    tools::sync_ring_vector<std::string> overwrite_range_vec(4U);
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result = overwrite_range_vec.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "sync overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << std::endl;
    std::cout << "sync overwrite range contents (recent history):" << std::endl;
    drain_sync_ring_vector(overwrite_range_vec);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // C++20: push_range via ranges concept
    {
        std::vector<std::string> batch = { "red", "green", "blue", "yellow" };
        auto filtered = batch | std::views::filter([](const auto& s) { return s.length() > 4; });
        const auto inserted = str_vec.push_range(filtered);
        std::cout << "inserted with C++20 filtered range: " << inserted << std::endl;
    }

    // C++20: pop_range via span
    {
        std::array<std::string, 3> buffer {};
        const auto popped = str_vec.pop_range(std::span(buffer));
        std::cout << "popped with C++20 span: " << popped << std::endl;
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << buffer[i] << std::endl;
        }
    }

    std::cout << "drain C++20 span batch:" << std::endl;
    drain_sync_ring_vector(str_vec);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_queue()
{
    std::cout << "-- sync queue --" << std::endl;
    tools::sync_queue<std::string> str_queue;

    // emplace: construct string in-place from a string literal
    str_queue.emplace("toto");

    auto item = str_queue.front_pop();
    if (item.has_value())
    {
        std::cout << *item << std::endl;
    }

    // push rvalue: move a pre-constructed string into the queue
    std::string s1 = "hello";
    std::string s2 = "world";
    str_queue.push(std::move(s1));
    str_queue.push(std::move(s2));

    std::cout << "size after two rvalue pushes: " << str_queue.size() << std::endl;

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << std::endl;
        }
    }

    // push_range (C++17): iterator-pair batch insert under a single lock
    std::vector<std::string> batch = { "alpha", "beta", "gamma", "delta" };
    str_queue.push_range(batch.begin(), batch.end());
    std::cout << "size after push_range (iterator-pair): " << str_queue.size() << std::endl;

    // pop_range (C++17): iterator-pair batch extraction under a single lock
    std::array<std::string, 3> popped_batch {};
    const auto popped_count = str_queue.pop_range(popped_batch.begin(), popped_batch.end());
    std::cout << "popped with pop_range (iterator-pair): " << popped_count << std::endl;
    for (std::size_t i = 0; i < popped_count; ++i)
    {
        std::cout << "  popped: " << popped_batch[i] << std::endl;
    }

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << std::endl;
        }
    }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload — accepts any std::ranges::input_range
    std::vector<std::string> range_batch = { "one", "two", "three" };
    str_queue.push_range(range_batch); // lvalue range
    std::cout << "size after push_range (C++20 range, lvalue): " << str_queue.size() << std::endl;

    // pop_range (C++20): span-based batch extraction
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with pop_range (C++20 span): " << popped_span_count << std::endl;
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  popped: " << popped_span[i] << std::endl;
    }

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << std::endl;
        }
    }

    // push_range with a range view: filter elements on the fly before enqueuing
    std::vector<std::string> mixed = { "keep_me", "skip", "keep_too", "nope" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    str_queue.push_range(filtered);
    std::cout << "size after push_range (C++20 filtered view): " << str_queue.size() << std::endl;

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << std::endl;
        }
    }
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_dictionary()
{
    std::cout << "-- sync dictionary --" << std::endl;
    tools::sync_dictionary<std::string, std::string> str_dict;

    // single add/find/remove path
    str_dict.add("toto", "blob");

    auto result = str_dict.find("toto");

    if (result.has_value())
    {
        std::cout << *result << std::endl;
        str_dict.remove("toto");
    }

    // add_range (C++17): iterator-pair batch insertion
    std::vector<std::pair<std::string, std::string>> batch = {
        { "k1", "v1" },
        { "k2", "v2" },
        { "k3", "v3" },
    };
    const auto inserted_pair = str_dict.add_range(batch.begin(), batch.end());
    std::cout << "add_range iterator-pair inserted: " << inserted_pair << std::endl;

    auto snapshot = str_dict.get_collection();
    std::cout << "dictionary snapshot after iterator-pair:" << std::endl;
    for (const auto& [key, value] : snapshot)
    {
        std::cout << "  " << key << " => " << value << std::endl;
    }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // add_range (C++20): range overload with a container
    std::vector<std::pair<std::string, std::string>> extra = {
        { "k4", "v4" },
        { "k5", "v5" },
        { "keep_A", "va" },
        { "drop_A", "vb" },
    };
    const auto inserted_range = str_dict.add_range(extra);
    std::cout << "add_range C++20 range inserted: " << inserted_range << std::endl;

    // add_range (C++20): range overload with a filtered view
    auto filtered = extra | std::views::filter([](const auto& kv) { return kv.first.starts_with("keep"); });
    const auto inserted_view = str_dict.add_range(filtered);
    std::cout << "add_range C++20 filtered view inserted: " << inserted_view << std::endl;
#endif

    snapshot = str_dict.get_collection();
    std::cout << "dictionary final snapshot (size=" << snapshot.size() << "):" << std::endl;
    for (const auto& [key, value] : snapshot)
    {
        std::cout << "  " << key << " => " << value << std::endl;
    }

    str_dict.clear();
    std::cout << "dictionary empty after clear: " << std::boolalpha << str_dict.empty() << std::noboolalpha
              << std::endl;
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_histogram()
{
    std::cout << "-- histogram --" << std::endl;
    tools::histogram<double> hist;

    hist.add(1.0);
    hist.emplace(2.0);

    // add_range (C++17): iterator-pair batch insertion
    std::vector<double> samples = { 1.0, 2.0, 2.0, 3.5, 3.5, 3.5 };
    const auto inserted_pair = hist.add_range(samples.begin(), samples.end());
    std::cout << "add_range iterator-pair inserted: " << inserted_pair << std::endl;

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // add_range (C++20): range overload with a container
    std::vector<double> extra = { -2.0, -1.0, 0.0, 0.5, 4.0, 5.0 };
    const auto inserted_range = hist.add_range(extra);
    std::cout << "add_range C++20 range inserted: " << inserted_range << std::endl;

    // add_range (C++20): range overload with a filtered view
    auto non_negative = extra | std::views::filter([](double value) { return value >= 0.0; });
    const auto inserted_view = hist.add_range(non_negative);
    std::cout << "add_range C++20 filtered view inserted: " << inserted_view << std::endl;
#endif

    const auto avg = hist.average();
    const auto var = hist.variance(avg);
    std::cout << "hist total count: " << hist.total_count() << std::endl;
    std::cout << "hist top value: " << hist.top() << " (" << hist.top_occurence() << " times)" << std::endl;
    std::cout << "hist avg: " << avg << " median: " << hist.median() << " variance: " << var << std::endl;
}

//--------------------------------------------------------------------------------------------------------------------------------

enum class my_topic
{
    generic,
    system,
    external
};

using base_observer = tools::sync_observer<my_topic, std::string>;
class my_observer : public base_observer
{
public:
    my_observer() = default;
    virtual ~my_observer()
    {
    }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "sync [topic " << static_cast<std::underlying_type<my_topic>::type>(topic) << "] received: event ("
                  << event << ") from " << origin << std::endl;
    }

private:
};

using base_async_observer = tools::async_observer<my_topic, std::string>;
class my_async_observer : public base_async_observer
{
public:
    my_async_observer()
        : m_task_loop([this]() { handle_events(); })
    {
    }

    virtual ~my_async_observer()
    {
        m_stop_task.store(true);
        m_task_loop.join();
    }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        std::cout << "async/push [topic " << static_cast<std::underlying_type<my_topic>::type>(topic)
                  << "] received: event (" << event << ") from " << origin << std::endl;

        base_async_observer::inform(topic, event, origin);
    }

private:
    void handle_events()
    {
        const auto timeout = std::chrono::duration<int, std::micro>(1000);

        while (!m_stop_task.load())
        {
            wait_for_events(timeout);

            while (number_of_events() > 0)
            {
                auto entry = pop_first_event();
                if (entry.has_value())
                {
                    auto& [topic, event, origin] = *entry;

                    std::cout << "async/pop [topic " << static_cast<std::underlying_type<my_topic>::type>(topic)
                              << "] received: event (" << event << ") from " << origin << std::endl;
                }
            }
        }
    }

    std::thread m_task_loop;
    std::atomic_bool m_stop_task = false;
};

using base_subject = tools::sync_subject<my_topic, std::string>;
class my_subject : public base_subject
{
public:
    my_subject() = delete;
    my_subject(const std::string& name)
        : base_subject(name)
    {
    }

    virtual ~my_subject()
    {
    }

    virtual void publish(const my_topic& topic, const std::string& event) const override
    {
        std::cout << "publish: event (" << event << ") to " << name() << std::endl;
        base_subject::publish(topic, event);
    }

private:
};

void test_publish_subscribe()
{
    std::cout << "-- publish subscribe --" << std::endl;
    auto observer1 = std::make_shared<my_observer>();
    auto observer2 = std::make_shared<my_observer>();
    auto async_observer = std::make_shared<my_async_observer>();
    auto subject1 = std::make_shared<my_subject>("source1");
    auto subject2 = std::make_shared<my_subject>("source2");

    subject1->subscribe(my_topic::generic, observer1);
    subject1->subscribe(my_topic::generic, observer2);
    subject1->subscribe(my_topic::system, observer2);
    subject1->subscribe(my_topic::generic, async_observer);

    subject2->subscribe(my_topic::generic, observer1);
    subject2->subscribe(my_topic::generic, observer2);
    subject2->subscribe(my_topic::system, observer2);
    subject2->subscribe(my_topic::generic, async_observer);

    subject1->subscribe(my_topic::generic, "loose_coupled_handler_1",
        [](const my_topic& topic, const std::string& event, const std::string& origin)
        {
            std::cout << "handler [topic " << static_cast<std::underlying_type<my_topic>::type>(topic)
                      << "] received: event (" << event << ") from " << origin << std::endl;
        });

    subject1->publish(my_topic::generic, "toto");

    subject1->unsubscribe(my_topic::generic, observer1);

    subject1->publish(my_topic::generic, "titi");

    subject1->publish(my_topic::system, "tata");

    subject1->unsubscribe(my_topic::generic, "loose_coupled_handler_1");

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(500));

    subject1->publish(my_topic::generic, "tintin");

    subject2->publish(my_topic::generic, "tonton");
    subject2->publish(my_topic::system, "tantine");
}

//--------------------------------------------------------------------------------------------------------------------------------

struct my_periodic_task_context
{
    std::atomic<int> loop_counter = 0;
    tools::sync_queue<std::chrono::high_resolution_clock::time_point> time_points;
};

using my_periodic_task = tools::periodic_task<my_periodic_task_context>;

void test_periodic_task()
{
    std::cout << "-- periodic task --" << std::endl;
    auto lambda = [](std::shared_ptr<my_periodic_task_context> context, const std::string& task_name) -> void
    {
        (void)task_name;
        context->loop_counter += 1;
        context->time_points.emplace(std::chrono::high_resolution_clock::now());
    };

    auto context = std::make_shared<my_periodic_task_context>();
    // 20 ms period
    constexpr const auto period = std::chrono::duration<int, std::micro>(20000);
    const auto start_timepoint = std::chrono::high_resolution_clock::now();
    my_periodic_task task1(lambda, context, "periodic task 1", period);

    // sleep 2 sec
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));

    std::cout << "nb of periodic loops = " << context->loop_counter.load() << std::endl;

    auto previous_timepoint = start_timepoint;
    while (!context->time_points.empty())
    {
        const auto measured_timepoint = context->time_points.front_pop();

        if (measured_timepoint.has_value())
        {
            const auto elapsed
                = std::chrono::duration_cast<std::chrono::microseconds>(*measured_timepoint - previous_timepoint);
            std::cout << "timepoint: " << elapsed.count() << " us" << std::endl;
            previous_timepoint = *measured_timepoint;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

class my_collector : public base_observer
{
public:
    my_collector() = default;
    virtual ~my_collector()
    {
    }

    virtual void inform(const my_topic& topic, const std::string& event, const std::string& origin) override
    {
        (void)topic;
        (void)origin;

        m_histogram.add(static_cast<double>(std::strtod(event.c_str(), nullptr)));
    }

    void display_stats()
    {
        const auto top = m_histogram.top();
        std::cout << std::endl
                  << "value " << top << " appears " << m_histogram.top_occurence() << " times" << std::endl;
        const auto avg = m_histogram.average();
        std::cout << "average value is " << avg << std::endl;
        std::cout << "median value is " << m_histogram.median() << std::endl;
        const auto variance = m_histogram.variance(avg);
        std::cout << "variance is " << variance << std::endl;
        const auto std_deviation = m_histogram.standard_deviation(variance);
        std::cout << "standard deviation is " << std_deviation << std::endl;
        std::cout << "gaussian probability of [" << std::floor(top) << "," << std::ceil(top) << "] occuring is "
                  << m_histogram.gaussian_probability(std::floor(top), std::ceil(top), avg, std_deviation, 100)
                  << std::endl;
    }

private:
    tools::histogram<double> m_histogram;
};

//--------------------------------------------------------------------------------------------------------------------------------

void test_periodic_publish_subscribe()
{
    std::cout << "-- periodic publish subscribe --" << std::endl;
    auto monitoring = std::make_shared<my_async_observer>();
    auto data_source = std::make_shared<my_subject>("data_source");
    auto histogram_feeder = std::make_shared<my_collector>();

    auto sampler
        = [&data_source](std::shared_ptr<my_periodic_task_context> context, const std::string& task_name) -> void
    {
        (void)task_name;

        context->loop_counter += 1;

        // mocked signal
        double signal = std::sin(context->loop_counter.load());

        // emit "signal" as a 'string' event
        data_source->publish(my_topic::external, std::to_string(signal));
    };

    data_source->subscribe(my_topic::external, monitoring);
    data_source->subscribe(my_topic::external, histogram_feeder);

    // "sample" with a 100 ms period
    auto context = std::make_shared<my_periodic_task_context>();
    const auto period = std::chrono::duration<int, std::milli>(100);
    {
        my_periodic_task periodic_task(sampler, context, "periodic task 1", period);

        std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));
    }

    histogram_feeder->display_stats();
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_queued_commands()
{
    std::cout << "-- queued commands --" << std::endl;
    tools::sync_queue<std::function<void()>> commands_queue;

    commands_queue.emplace([]() { std::cout << "hello" << std::endl; });

    commands_queue.emplace([]() { std::cout << "world" << std::endl; });

    while (!commands_queue.empty())
    {
        auto call = commands_queue.front_pop();
        if (call.has_value())
        {
            (*call)();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_ring_buffer_commands()
{
    std::cout << "-- ring buffer commands --" << std::endl;
    tools::sync_ring_buffer<std::function<void()>, 128U> commands_queue;

    commands_queue.emplace([]() { std::cout << "hello" << std::endl; });

    commands_queue.emplace([]() { std::cout << "world" << std::endl; });

    while (!commands_queue.empty())
    {
        auto call = commands_queue.front_pop();
        if (call.has_value())
        {
            (*call)();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

struct my_worker_task_context
{
    std::atomic<int> loop_counter = 0;
    tools::sync_queue<std::chrono::high_resolution_clock::time_point> time_points;
};

using my_worker_task = tools::worker_task<my_worker_task_context>;

void test_worker_tasks()
{
    std::cout << "-- worker tasks --" << std::endl;

    auto context = std::make_shared<my_worker_task_context>();

    auto task1 = std::make_unique<my_worker_task>(context, "worker_1");
    auto task2 = std::make_unique<my_worker_task>(context, "worker_2");

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 1);
    std::array<std::unique_ptr<my_worker_task>, 2> tasks = { std::move(task1), std::move(task2) };

    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(100)); // 100 ms

    const auto start_timepoint = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 20; ++i)
    {
        auto idx = distribution(generator);

        tasks[idx]->delegate(
            [](auto context, const auto& task_name) -> void
            {
                std::cout << "job " << context->loop_counter.load() << " on worker task " << task_name.c_str()
                          << std::endl;
                context->loop_counter++;
                context->time_points.emplace(std::chrono::high_resolution_clock::now());
            });

        std::this_thread::yield();
    }

    // delegate_range (C++17): iterator-pair batch delegation
    std::vector<my_worker_task::call_back> batch_jobs;
    batch_jobs.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        batch_jobs.emplace_back(
            [](auto context, const auto& task_name) -> void
            {
                std::cout << "batch job " << context->loop_counter.load() << " on worker task " << task_name.c_str()
                          << std::endl;
                context->loop_counter++;
                context->time_points.emplace(std::chrono::high_resolution_clock::now());
            });
    }
    tasks[0]->delegate_range(batch_jobs.begin(), batch_jobs.end());

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // delegate_range (C++20): range overload with a container
    tasks[1]->delegate_range(batch_jobs);
#endif

    // sleep 2 sec
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(2000));

    std::cout << "nb of periodic loops = " << context->loop_counter.load() << std::endl;

    auto previous_timepoint = start_timepoint;
    while (!context->time_points.empty())
    {
        const auto measured_timepoint = context->time_points.front_pop();

        if (measured_timepoint.has_value())
        {
            const auto elapsed
                = std::chrono::duration_cast<std::chrono::microseconds>(*measured_timepoint - previous_timepoint);
            std::cout << "timepoint: " << elapsed.count() << " us" << std::endl;
            previous_timepoint = *measured_timepoint;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------
namespace
{
    constexpr std::size_t ALLOC_MAX_SIZE = 512;
    constexpr std::size_t ALLOC_ITERATIONS = 10000000;

    enum class alloc_type
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

#if defined(USE_MEM_POOL_ALLOCATOR)
// https://www.rastergrid.com/blog/sw-eng/2021/03/custom-memory-allocators/
extern void init_mem_pool_allocator();
extern void destroy_mem_pool_allocator();
#endif

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

#if defined(USE_MEM_POOL_ALLOCATOR)
    // prevent memory fragmentation with frequent heap allocations of
    // small events/messages
    init_mem_pool_allocator();
#endif

    test_ring_buffer();
    test_lock_free_ring_buffer();
    test_sync_ring_buffer();
    test_ring_vector();
    test_sync_ring_vector();
    test_sync_queue();
    test_sync_dictionary();
    test_histogram();

    test_publish_subscribe();
    test_periodic_task();
    test_periodic_publish_subscribe();

    test_queued_commands();
    test_ring_buffer_commands();
    test_worker_tasks();

    test_allocator_stress();

#if defined(USE_MEM_POOL_ALLOCATOR)
    destroy_mem_pool_allocator();
#endif

    return 0;
}
