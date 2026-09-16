/**
 * @file example_sync_container.cpp
 * @brief Runs synchronized container and expected examples.
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

void test_sync_queue()
{
    std::cout << "-- sync queue --" << '\n';
    tools::sync_queue<std::string> str_queue;

    // emplace: construct string in-place from a string literal
    str_queue.emplace("toto");

    auto item = str_queue.front_pop();
    if (item.has_value())
    {
        std::cout << *item << '\n';
    }

    // push rvalue: move a pre-constructed string into the queue
    std::string s1 = "hello";
    std::string s2 = "world";
    str_queue.push(std::move(s1));
    str_queue.push(std::move(s2));

    std::cout << "size after two rvalue pushes: " << str_queue.size() << '\n';

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << '\n';
        }
    }

    // push_range (C++17): iterator-pair batch insert under a single lock
    std::vector<std::string> batch = { "alpha", "beta", "gamma", "delta" };
    str_queue.push_range(batch.begin(), batch.end());
    std::cout << "size after push_range (iterator-pair): " << str_queue.size() << '\n';

    // pop_range (C++17): iterator-pair batch extraction under a single lock
    std::array<std::string, 3> popped_batch {};
    const auto popped_count = str_queue.pop_range(popped_batch.begin(), popped_batch.end());
    std::cout << "popped with pop_range (iterator-pair): " << popped_count << '\n';
    for (std::size_t i = 0; i < popped_count; ++i)
    {
        std::cout << "  popped: " << popped_batch.at(i) << '\n';
    }

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << '\n';
        }
    }

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // push_range (C++20): range overload — accepts any std::ranges::input_range
    std::vector<std::string> range_batch = { "one", "two", "three" };
    str_queue.push_range(range_batch); // lvalue range
    std::cout << "size after push_range (C++20 range, lvalue): " << str_queue.size() << '\n';

    // pop_range (C++20): span-based batch extraction
    std::array<std::string, 4> popped_span {};
    const auto popped_span_count = str_queue.pop_range(std::span<std::string>(popped_span));
    std::cout << "popped with pop_range (C++20 span): " << popped_span_count << '\n';
    for (std::size_t i = 0; i < popped_span_count; ++i)
    {
        std::cout << "  popped: " << popped_span.at(i) << '\n';
    }

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << '\n';
        }
    }

    // push_range with a range view: filter elements on the fly before enqueuing
    std::vector<std::string> mixed = { "keep_me", "skip", "keep_too", "nope" };
    auto filtered = mixed | std::views::filter([](const std::string& s) { return s.starts_with("keep"); });
    str_queue.push_range(filtered);
    std::cout << "size after push_range (C++20 filtered view): " << str_queue.size() << '\n';

    while (!str_queue.empty())
    {
        auto val = str_queue.front_pop();
        if (val.has_value())
        {
            std::cout << "  popped: " << *val << '\n';
        }
    }
#endif
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_priority_queue()
{
    std::cout << "-- sync priority queue --" << '\n';

    tools::sync_priority_queue<int> min_heap;
    min_heap.push(5);
    min_heap.push(1);
    min_heap.push(3);

    std::cout << "min-heap pop order:" << '\n';
    while (!min_heap.empty())
    {
        auto item = min_heap.top_pop();
        if (item.has_value())
        {
            std::cout << "  " << *item << '\n';
        }
    }

    enum class pq_topic : std::uint8_t
    {
        generic
    };

    struct priority_event
    {
        int priority;
        std::string message;

        bool operator<(const priority_event& other) const
        {
            return priority < other.priority;
        }
    };

    // async_observer can swap sync_queue for sync_priority_queue transparently.
    tools::async_observer<pq_topic, priority_event, tools::sync_priority_queue> observer;

    observer.inform(pq_topic::generic, priority_event { .priority = 3, .message = "low" }, "worker");
    observer.inform(pq_topic::generic, priority_event { .priority = 1, .message = "high" }, "worker");
    observer.inform(pq_topic::generic, priority_event { .priority = 2, .message = "medium" }, "worker");

    auto events = observer.pop_all_events();
    std::cout << "async_observer priority order:" << '\n';
    for (const auto& evt : events)
    {
        std::cout << "  p=" << std::get<1>(evt).priority << " msg=" << std::get<1>(evt).message << '\n';
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_async_observer_queue_overflow()
{
    std::cout << "-- async observer queue overflow --" << '\n';

    tools::async_observer<std::string, std::string, tools::sync_ring_vector> observer(2U);
    observer.inform("topic", "event-1", "producer");
    observer.inform("topic", "event-2", "producer");
    observer.inform("topic", "event-3-dropped", "producer");

    std::cout << "overflow detected = " << std::boolalpha << observer.has_queue_overflow()
              << ", dropped = " << observer.queue_overflow_count() << std::noboolalpha << '\n';

    const auto dropped_count = observer.consume_queue_overflow_count();
    std::cout << "consumed dropped count = " << dropped_count << ", overflow pending = " << std::boolalpha
              << observer.has_queue_overflow() << std::noboolalpha << '\n';

    const auto events = observer.pop_all_events();
    std::cout << "queued events = " << events.size() << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_dictionary()
{
    std::cout << "-- sync dictionary --" << '\n';
    tools::sync_dictionary<std::string, std::string> str_dict;

    // single add/find/remove path
    str_dict.add("toto", "blob");

    auto result = str_dict.find("toto");

    if (result.has_value())
    {
        std::cout << *result << '\n';
        str_dict.remove("toto");
    }

    // add_range (C++17): iterator-pair batch insertion
    std::vector<std::pair<std::string, std::string>> batch = {
        { "k1", "v1" },
        { "k2", "v2" },
        { "k3", "v3" },
    };
    const auto inserted_pair = str_dict.add_range(batch.begin(), batch.end());
    std::cout << "add_range iterator-pair inserted: " << inserted_pair << '\n';

    auto snapshot = str_dict.get_collection();
    std::cout << "dictionary snapshot after iterator-pair:" << '\n';
    for (const auto& [key, value] : snapshot)
    {
        std::cout << "  " << key << " => " << value << '\n';
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
    std::cout << "add_range C++20 range inserted: " << inserted_range << '\n';

    // add_range (C++20): range overload with a filtered view
    auto filtered = extra | std::views::filter([](const auto& kv) { return kv.first.starts_with("keep"); });
    const auto inserted_view = str_dict.add_range(filtered);
    std::cout << "add_range C++20 filtered view inserted: " << inserted_view << '\n';
#endif

    snapshot = str_dict.get_collection();
    std::cout << "dictionary final snapshot (size=" << snapshot.size() << "):" << '\n';
    for (const auto& [key, value] : snapshot)
    {
        std::cout << "  " << key << " => " << value << '\n';
    }

    // heterogeneous lookup: query/erase a std::string-keyed dictionary with std::string_view,
    // avoiding a temporary std::string construction
    const std::string_view view_key = "k1";
    std::cout << "contains(string_view k1) = " << std::boolalpha << str_dict.contains(view_key) << std::noboolalpha
              << '\n';

    const auto view_result = str_dict.find(view_key);
    if (view_result.has_value())
    {
        std::cout << "find(string_view k1) = " << *view_result << '\n';
    }

    str_dict.remove(view_key);
    std::cout << "contains(string_view k1) after remove = " << std::boolalpha << str_dict.contains(view_key)
              << std::noboolalpha << '\n';

    str_dict.clear();
    std::cout << "dictionary empty after clear: " << std::boolalpha << str_dict.empty() << std::noboolalpha << '\n';

    // same API, alternate backing container.
    tools::sync_dictionary<std::string, std::string, std::unordered_map<std::string, std::string>> hash_dict;
    hash_dict.add("u1", "one");
    hash_dict.add("u2", "two");
    auto hash_snapshot = hash_dict.snapshot();
    std::cout << "unordered_map-backed dictionary size=" << hash_snapshot.size() << " contains(u2)=" << std::boolalpha
              << hash_dict.contains("u2") << std::noboolalpha << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_expected()
{
    std::cout << "-- expected --" << '\n';

    auto parse_positive = [](int value) -> tools::expected<int, std::string>
    {
        if (value < 0)
        {
            return tools::unexpected<std::string>("value must be >= 0");
        }
        return value * 2;
    };

    auto ok = parse_positive(21);
    if (ok.has_value())
    {
        std::cout << "ok value=" << ok.value() << '\n';
    }

    auto ko = parse_positive(-1);
    if (!ko.has_value())
    {
        std::cout << "error=" << ko.error() << '\n';
    }

    auto validate_name = [](const std::string& name) -> tools::expected<void, std::string>
    {
        if (name.empty())
        {
            return tools::unexpected<std::string>("name is empty");
        }
        return {};
    };

    auto status = validate_name("");
    if (!status)
    {
        std::cout << "void-error=" << status.error() << '\n';
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_expected_unit_style()
{
    std::cout << "-- expected unit-style checks --" << '\n';

    std::size_t passed = 0U;
    std::size_t failed = 0U;

    auto check = [&passed, &failed](bool condition, const char* name)
    {
        if (condition)
        {
            ++passed;
            std::cout << "  [PASS] " << name << '\n';
        }
        else
        {
            ++failed;
            std::cout << "  [FAIL] " << name << '\n';
        }
    };

    auto parse_positive = [](int value) -> tools::expected<int, std::string>
    {
        if (value < 0)
        {
            return tools::unexpected<std::string>("negative");
        }
        return value;
    };

    {
        auto result = parse_positive(7);
        check(result.has_value(), "success has_value");
        check(static_cast<bool>(result), "success bool conversion");
        check(result.value() == 7, "success value");
        check(result.value_or(42) == 7, "success value_or keeps value");
    }

    {
        auto result = parse_positive(-3);
        check(!result.has_value(), "error has_value");
        check(!static_cast<bool>(result), "error bool conversion");
        check(result.error() == "negative", "error payload");
        check(result.value_or(42) == 42, "error value_or fallback");
    }

    {
        tools::expected<int, std::string> result(tools::unexpect, "bad parse");
        check(!result.has_value(), "unexpect constructor");
        check(result.error() == "bad parse", "unexpect error payload");
    }

    auto validate_name = [](const std::string& name) -> tools::expected<void, std::string>
    {
        if (name.empty())
        {
            return tools::unexpected<std::string>("empty");
        }
        return {};
    };

    {
        auto status_ok = validate_name("alice");
        check(status_ok.has_value(), "void success has_value");
        check(static_cast<bool>(status_ok), "void success bool conversion");
        status_ok.value();
        check(true, "void success value() precondition");
    }

    {
        auto status_error = validate_name("");
        check(!status_error.has_value(), "void error has_value");
        check(status_error.error() == "empty", "void error payload");
    }

#if defined(TOOLS_HAS_STD_EXPECTED)
    std::cout << "  backend: std::expected" << std::endl;
#else
    std::cout << "  backend: tools fallback expected" << '\n';
#endif

    std::cout << "  summary: passed=" << passed << " failed=" << failed << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------


void run_example_sync_container()
{
    test_sync_queue();
    test_sync_priority_queue();
    test_async_observer_queue_overflow();
    test_sync_dictionary();
    test_expected();
    test_expected_unit_style();
}
