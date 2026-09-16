/**
 * @file example_ring_container.cpp
 * @brief Runs ring buffer and ring vector examples.
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

void test_ring_buffer()
{
    std::cout << "-- ring buffer --" << '\n';
    tools::ring_buffer<std::string, 64U> str_queue;

    // emplace: construct string directly in the buffer
    str_queue.emplace("toto");

    // push rvalue: move a pre-constructed string into the buffer
    std::string moved = "titi";
    str_queue.push(std::move(moved));

    auto item = str_queue.front();

    std::cout << "front after emplace/push: " << item << '\n';

    std::cout << "drain initial content:" << '\n';
    drain_ring_buffer(str_queue);

    // push_range (C++17): iterator-pair insertion
    std::vector<std::string> batch = { "alpha", "beta", "gamma" };
    const auto inserted_pair = str_queue.push_range(batch.begin(), batch.end());
    std::cout << "inserted with iterator-pair: " << inserted_pair << '\n';

    // pop_range (C++17): iterator-pair batch extraction
    std::array<std::string, 2> popped_pair {};
    const auto popped_pair_count = str_queue.pop_range(popped_pair.begin(), popped_pair.end());
    std::cout << "popped with iterator-pair: " << popped_pair_count << '\n';
    for (std::size_t i = 0; i < popped_pair_count; ++i)
    {
        std::cout << "  " << popped_pair.at(i) << '\n';
    }

    std::cout << "drain iterator-pair batch:" << '\n';
    drain_ring_buffer(str_queue);

    // capacity-bound behavior: insertion stops when full
    tools::ring_buffer<std::string, 4U> small_queue;
    std::vector<std::string> overflow_batch = { "A", "B", "C", "D", "E" };
    const auto inserted_limited = small_queue.push_range(overflow_batch.begin(), overflow_batch.end());
    std::cout << "inserted in capacity-limited buffer: " << inserted_limited << " / " << overflow_batch.size() << '\n';
    std::cout << "small queue full: " << std::boolalpha << small_queue.full() << std::noboolalpha << '\n';

    std::cout << "drain capacity-limited buffer:" << '\n';
    drain_ring_buffer(small_queue);

    // reject-on-full mode (single push)
    tools::ring_buffer<std::string, 4U> reject_queue;
    reject_queue.push("R1");
    reject_queue.push("R2");
    reject_queue.push("R3");
    const bool reject_single_ok = reject_queue.push("R4");
    std::cout << "reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << '\n';
    std::cout << "reject mode contents:" << '\n';
    drain_ring_buffer(reject_queue);

    // overwrite-on-full mode (single push)
    tools::ring_buffer<std::string, 4U> overwrite_queue;
    overwrite_queue.push("O1");
    overwrite_queue.push("O2");
    overwrite_queue.push("O3");
    const bool overwrite_single_happened = overwrite_queue.push_overwrite("O4");
    std::cout << "overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << '\n';
    std::cout << "overwrite mode contents (recent history):" << '\n';
    drain_ring_buffer(overwrite_queue);

    // overwrite-on-full mode (push_range)
    tools::ring_buffer<std::string, 4U> overwrite_range_queue;
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_queue.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << '\n';
    std::cout << "overwrite range contents (recent history):" << '\n';
    drain_ring_buffer(overwrite_range_queue);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a container
    std::vector<std::string> range_batch = { "one", "two", "three" };
    const auto inserted_range = str_queue.push_range(range_batch);
    std::cout << "inserted with C++20 range: " << inserted_range << '\n';

    // pop_range (C++20): span-based batch extraction
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with C++20 span: " << popped_span_count << '\n';
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  " << popped_span.at(i) << '\n';
    }

    std::cout << "drain C++20 container range:" << '\n';
    drain_ring_buffer(str_queue);

    // push_range (C++20): range overload with a filtered view
    std::vector<std::string> mixed = { "keep_1", "skip", "keep_2", "no" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    const auto inserted_view = str_queue.push_range(filtered);
    std::cout << "inserted with C++20 filtered view: " << inserted_view << '\n';

    std::cout << "drain C++20 filtered view:" << '\n';
    drain_ring_buffer(str_queue);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_lock_free_ring_buffer()
{
    std::cout << "-- lock free ring buffer --" << '\n';
    tools::lock_free_ring_buffer<int, 4U> queue;

    // push_range (C++17): iterator-pair insertion
    std::vector<int> input = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    const auto inserted = queue.push_range(input.begin(), input.end());
    std::cout << "push_range inserted: " << inserted << '\n';

    // pop_range (C++17): iterator-pair extraction
    std::array<int, 6> popped_first {};
    const auto popped_count_first = queue.pop_range(popped_first.begin(), popped_first.end());
    std::cout << "pop_range iterator-pair popped: " << popped_count_first << '\n';
    for (std::size_t i = 0; i < popped_count_first; ++i)
    {
        std::cout << "  " << popped_first.at(i) << '\n';
    }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a filtered view
    std::vector<int> more = { 11, 12, 13, 14, 15, 16, 17, 18 };
    auto evens = more | std::views::filter([](int value) { return (value % 2) == 0; });
    const auto inserted_view = queue.push_range(evens);
    std::cout << "push_range C++20 filtered range inserted: " << inserted_view << '\n';

    // pop_range (C++20): span overload
    std::array<int, 8> popped_second {};
    const auto popped_count_second = queue.pop_range(std::span<int>(popped_second));
    std::cout << "pop_range C++20 span popped: " << popped_count_second << '\n';
    for (std::size_t i = 0; i < popped_count_second; ++i)
    {
        std::cout << "  " << popped_second.at(i) << '\n';
    }
#endif

    // small SPSC stress test: force wraparound repeatedly and verify FIFO ordering.
    tools::lock_free_ring_buffer<int, 3U> stress_queue;
    static constexpr int stress_item_count = 50000;
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
              << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_ring_buffer()
{
    std::cout << "-- sync ring buffer --" << '\n';
    tools::sync_ring_buffer<std::string, 64U> str_queue;

    // emplace + push(rvalue)
    str_queue.emplace("toto");
    std::string moved = "titi";
    str_queue.push(std::move(moved));

    std::cout << "drain initial sync content:" << '\n';
    drain_sync_ring_buffer(str_queue);

    // push_range (C++17): iterator-pair insertion under one lock
    std::vector<std::string> batch = { "alpha", "beta", "gamma" };
    const auto inserted_pair = str_queue.push_range(batch.begin(), batch.end());
    std::cout << "inserted with iterator-pair: " << inserted_pair << '\n';

    // pop_range (C++17): iterator-pair extraction under one lock
    std::array<std::string, 2> popped_pair {};
    const auto popped_pair_count = str_queue.pop_range(popped_pair.begin(), popped_pair.end());
    std::cout << "popped with iterator-pair: " << popped_pair_count << '\n';
    for (std::size_t i = 0; i < popped_pair_count; ++i)
    {
        std::cout << "  " << popped_pair.at(i) << '\n';
    }

    std::cout << "drain iterator-pair sync batch:" << '\n';
    drain_sync_ring_buffer(str_queue);

    // capacity-bound behavior: insertion stops when full
    tools::sync_ring_buffer<std::string, 4U> small_queue;
    std::vector<std::string> overflow_batch = { "A", "B", "C", "D", "E" };
    const auto inserted_limited = small_queue.push_range(overflow_batch.begin(), overflow_batch.end());
    std::cout << "inserted in capacity-limited sync buffer: " << inserted_limited << " / " << overflow_batch.size()
              << '\n';
    std::cout << "small sync queue full: " << std::boolalpha << small_queue.full() << std::noboolalpha << '\n';

    std::cout << "drain capacity-limited sync buffer:" << '\n';
    drain_sync_ring_buffer(small_queue);

    // reject-on-full mode (single push)
    tools::sync_ring_buffer<std::string, 4U> reject_queue;
    reject_queue.push("R1");
    reject_queue.push("R2");
    reject_queue.push("R3");
    const bool reject_single_ok = reject_queue.push("R4");
    std::cout << "sync reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << '\n';
    std::cout << "sync reject mode contents:" << '\n';
    drain_sync_ring_buffer(reject_queue);

    // overwrite-on-full mode (single push)
    tools::sync_ring_buffer<std::string, 4U> overwrite_queue;
    overwrite_queue.push("O1");
    overwrite_queue.push("O2");
    overwrite_queue.push("O3");
    const bool overwrite_single_happened = overwrite_queue.push_overwrite("O4");
    std::cout << "sync overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << '\n';
    std::cout << "sync overwrite mode contents (recent history):" << '\n';
    drain_sync_ring_buffer(overwrite_queue);

    // overwrite-on-full mode (push_range)
    tools::sync_ring_buffer<std::string, 4U> overwrite_range_queue;
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_queue.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "sync overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << '\n';
    std::cout << "sync overwrite range contents (recent history):" << '\n';
    drain_sync_ring_buffer(overwrite_range_queue);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload with a container
    std::vector<std::string> range_batch = { "one", "two", "three" };
    const auto inserted_range = str_queue.push_range(range_batch);
    std::cout << "inserted with C++20 range: " << inserted_range << '\n';

    // pop_range (C++20): span-based extraction under one lock
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with C++20 span: " << popped_span_count << '\n';
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  " << popped_span.at(i) << '\n';
    }

    std::cout << "drain C++20 container sync range:" << '\n';
    drain_sync_ring_buffer(str_queue);

    // push_range (C++20): range overload with a filtered view
    std::vector<std::string> mixed = { "keep_1", "skip", "keep_2", "no" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    const auto inserted_view = str_queue.push_range(filtered);
    std::cout << "inserted with C++20 filtered view: " << inserted_view << '\n';

    std::cout << "drain C++20 filtered sync view:" << '\n';
    drain_sync_ring_buffer(str_queue);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_ring_vector()
{
    std::cout << "-- ring vector --" << '\n';
    tools::ring_vector<std::string> str_vec(10U);

    // emplace: construct string directly in the vector
    str_vec.emplace("alpha");

    // push rvalue: move a pre-constructed string into the vector
    std::string moved = "beta";
    str_vec.push(std::move(moved));

    auto item = str_vec.front();
    std::cout << "front after emplace/push: " << item << '\n';

    std::cout << "drain initial content:" << '\n';
    drain_ring_vector(str_vec);

    // C++17: push_range via iterator-pair insertion
    {
        std::vector<std::string> batch = { "apple", "banana", "cherry" };
        const auto inserted = str_vec.push_range(batch.begin(), batch.end());
        std::cout << "inserted with iterator-pair: " << inserted << '\n';
    }

    // C++17: pop_range via iterator-pair extraction
    {
        std::array<std::string, 2> output {};
        const auto popped = str_vec.pop_range(output.begin(), output.end());
        std::cout << "popped with iterator-pair: " << popped << '\n';
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << output.at(i) << '\n';
        }
    }

    std::cout << "drain iterator-pair batch:" << '\n';
    drain_ring_vector(str_vec);

    // reject-on-full mode (single push)
    tools::ring_vector<std::string> reject_vec(4U);
    reject_vec.push("R1");
    reject_vec.push("R2");
    reject_vec.push("R3");
    reject_vec.push("R4");
    const bool reject_single_ok = reject_vec.push("R5");
    std::cout << "reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << '\n';
    std::cout << "reject mode contents:" << '\n';
    drain_ring_vector(reject_vec);

    // overwrite-on-full mode (single push)
    tools::ring_vector<std::string> overwrite_vec(4U);
    overwrite_vec.push("O1");
    overwrite_vec.push("O2");
    overwrite_vec.push("O3");
    overwrite_vec.push("O4");
    const bool overwrite_single_happened = overwrite_vec.push_overwrite("O5");
    std::cout << "overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << '\n';
    std::cout << "overwrite mode contents (recent history):" << '\n';
    drain_ring_vector(overwrite_vec);

    // overwrite-on-full mode (push_range)
    tools::ring_vector<std::string> overwrite_range_vec(4U);
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_vec.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << '\n';
    std::cout << "overwrite range contents (recent history):" << '\n';
    drain_ring_vector(overwrite_range_vec);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // C++20: push_range via ranges concept
    {
        std::vector<std::string> batch = { "dog", "elephant", "fox", "giraffe" };
        auto filtered = batch | std::views::filter([](const auto& s) { return s.length() > 3; });
        const auto inserted = str_vec.push_range(filtered);
        std::cout << "inserted with C++20 filtered range: " << inserted << '\n';
    }

    // C++20: pop_range via span
    {
        std::array<std::string, 3> buffer {};
        const auto popped = str_vec.pop_range(std::span(buffer));
        std::cout << "popped with C++20 span: " << popped << '\n';
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << buffer.at(i) << '\n';
        }
    }

    std::cout << "drain C++20 span batch:" << '\n';
    drain_ring_vector(str_vec);
#endif

    // Resize test: expand capacity and add more items
    {
        std::cout << "resize test:" << '\n';
        str_vec.emplace("item1");
        str_vec.emplace("item2");
        str_vec.emplace("item3");
        std::cout << "before resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << '\n';
        str_vec.resize(20U);
        std::cout << "after expand resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << '\n';

        // Add more items after resize
        std::vector<std::string> more = { "new1", "new2", "new3", "new4" };
        str_vec.push_range(more.begin(), more.end());
        std::cout << "after push_range: size=" << str_vec.size() << '\n';

        // Shrink and check if oldest items are dropped
        std::cout << "contents before shrink:" << '\n';
        for (std::size_t i = 0; i < str_vec.size(); ++i)
        {
            std::cout << "  [" << i << "] " << str_vec[i] << '\n';
        }

        str_vec.resize(4U);
        std::cout << "after shrink resize: size=" << str_vec.size() << ", capacity=" << str_vec.capacity() << '\n';
        std::cout << "contents after shrink:" << '\n';
        for (std::size_t i = 0; i < str_vec.size(); ++i)
        {
            std::cout << "  [" << i << "] " << str_vec[i] << '\n';
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_ring_vector()
{
    std::cout << "-- sync ring vector --" << '\n';
    tools::sync_ring_vector<std::string> str_vec(10U);

    // emplace: construct string directly in the vector
    str_vec.emplace("initial");

    auto item = str_vec.front();
    if (item.has_value())
    {
        std::cout << "front after emplace: " << *item << '\n';
    }

    // drain initial content
    std::cout << "drain initial content:" << '\n';
    drain_sync_ring_vector(str_vec);

    // C++17: push_range via iterator-pair
    {
        std::vector<std::string> batch = { "one", "two", "three" };
        const auto inserted = str_vec.push_range(batch.begin(), batch.end());
        std::cout << "inserted with iterator-pair: " << inserted << '\n';
    }

    // C++17: pop_range via iterator-pair
    {
        std::array<std::string, 2> output {};
        const auto popped = str_vec.pop_range(output.begin(), output.end());
        std::cout << "popped with iterator-pair: " << popped << '\n';
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << output.at(i) << '\n';
        }
    }

    std::cout << "drain iterator-pair batch:" << '\n';
    drain_sync_ring_vector(str_vec);

    // reject-on-full mode (single push)
    tools::sync_ring_vector<std::string> reject_vec(4U);
    reject_vec.push("R1");
    reject_vec.push("R2");
    reject_vec.push("R3");
    reject_vec.push("R4");
    const bool reject_single_ok = reject_vec.push("R5");
    std::cout << "sync reject mode single push accepted extra item: " << std::boolalpha << reject_single_ok
              << std::noboolalpha << '\n';
    std::cout << "sync reject mode contents:" << '\n';
    drain_sync_ring_vector(reject_vec);

    // overwrite-on-full mode (single push)
    tools::sync_ring_vector<std::string> overwrite_vec(4U);
    overwrite_vec.push("O1");
    overwrite_vec.push("O2");
    overwrite_vec.push("O3");
    overwrite_vec.push("O4");
    const bool overwrite_single_happened = overwrite_vec.push_overwrite("O5");
    std::cout << "sync overwrite mode single push evicted oldest: " << std::boolalpha << overwrite_single_happened
              << std::noboolalpha << '\n';
    std::cout << "sync overwrite mode contents (recent history):" << '\n';
    drain_sync_ring_vector(overwrite_vec);

    // overwrite-on-full mode (push_range)
    tools::sync_ring_vector<std::string> overwrite_range_vec(4U);
    std::vector<std::string> overwrite_input = { "W1", "W2", "W3", "W4", "W5", "W6" };
    const auto overwrite_result
        = overwrite_range_vec.push_range_overwrite(overwrite_input.begin(), overwrite_input.end());
    std::cout << "sync overwrite range inserted=" << overwrite_result.inserted
              << " overwritten=" << overwrite_result.overwritten << '\n';
    std::cout << "sync overwrite range contents (recent history):" << '\n';
    drain_sync_ring_vector(overwrite_range_vec);

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // C++20: push_range via ranges concept
    {
        std::vector<std::string> batch = { "red", "green", "blue", "yellow" };
        auto filtered = batch | std::views::filter([](const auto& s) { return s.length() > 4; });
        const auto inserted = str_vec.push_range(filtered);
        std::cout << "inserted with C++20 filtered range: " << inserted << '\n';
    }

    // C++20: pop_range via span
    {
        std::array<std::string, 3> buffer {};
        const auto popped = str_vec.pop_range(std::span(buffer));
        std::cout << "popped with C++20 span: " << popped << '\n';
        for (std::size_t i = 0; i < popped; ++i)
        {
            std::cout << "  " << buffer.at(i) << '\n';
        }
    }

    std::cout << "drain C++20 span batch:" << '\n';
    drain_sync_ring_vector(str_vec);
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------


void run_example_ring_container()
{
    test_ring_buffer();
    test_lock_free_ring_buffer();
    test_sync_ring_buffer();
    test_ring_vector();
    test_sync_ring_vector();
}
