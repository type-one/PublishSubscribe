/**
 * @file example_time_list.cpp
 * @brief Runs chronological container and histogram examples.
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

void test_time_list()
{
    std::cout << "-- time_list --" << '\n';

    // Integral timestamp + string payload.
    tools::time_list<long, std::string> int_list;
    int_list.push(300L, "three hundred");
    int_list.push(100L, "one hundred");
    int_list.emplace(200L, "two hundred");

    auto earliest = int_list.top();
    if (earliest.has_value())
    {
        std::cout << "integral earliest: " << earliest->first << " => " << earliest->second << '\n';
    }

    auto sorted_snapshot = int_list.snapshot_sorted();
    std::cout << "integral snapshot order:" << '\n';
    for (const auto& entry : sorted_snapshot)
    {
        std::cout << "  " << entry.first << " => " << entry.second << '\n';
    }

    std::cout << "integral drain order:" << '\n';
    while (!int_list.empty())
    {
        auto entry = int_list.top_pop();
        if (entry.has_value())
        {
            std::cout << "  " << entry->first << " => " << entry->second << '\n';
        }
    }

    // Chrono timestamp + simple payload.
    using steady_tp = std::chrono::steady_clock::time_point;
    tools::time_list<steady_tp, int> chrono_list;
    const auto base_time = std::chrono::steady_clock::now();

    chrono_list.push(base_time + std::chrono::milliseconds(300), 3);
    chrono_list.push(base_time + std::chrono::milliseconds(100), 1);
    chrono_list.push(base_time + std::chrono::milliseconds(200), 2);

    std::cout << "chrono drain order (values should be 1,2,3):" << '\n';
    while (!chrono_list.empty())
    {
        auto entry = chrono_list.top_pop();
        if (entry.has_value())
        {
            std::cout << "  " << entry->second << '\n';
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_time_list_unit_style()
{
    std::cout << "-- time_list unit-style checks --" << '\n';

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

    tools::time_list<long, int> list;

    check(list.empty(), "starts empty");
    check(list.empty(), "starts with size 0");
    check(!list.top().has_value(), "top() empty returns nullopt");
    check(!list.top_pop().has_value(), "top_pop() empty returns nullopt");

    list.push(300L, 300);
    list.push(100L, 100);
    list.emplace(200L, 200);

    check(!list.empty(), "non-empty after inserts");
    check(list.size() == 3U, "size after inserts");

    auto top_entry = list.top();
    check(top_entry.has_value(), "top() returns value when non-empty");
    if (top_entry.has_value())
    {
        check(top_entry->first == 100L, "top() returns earliest timestamp");
        check(top_entry->second == 100, "top() returns expected payload");
    }

    auto snapshot = list.snapshot_sorted();
    check(snapshot.size() == 3U, "snapshot size matches");
    if (snapshot.size() == 3U)
    {
        check(snapshot[0].first == 100L, "snapshot[0] timestamp");
        check(snapshot[1].first == 200L, "snapshot[1] timestamp");
        check(snapshot[2].first == 300L, "snapshot[2] timestamp");
    }

    check(list.size() == 3U, "snapshot does not drain container");

    auto first = list.top_pop();
    auto second = list.top_pop();
    auto third = list.top_pop();

    check(first.has_value() && first->first == 100L, "top_pop order #1");
    check(second.has_value() && second->first == 200L, "top_pop order #2");
    check(third.has_value() && third->first == 300L, "top_pop order #3");
    check(list.empty(), "empty after full drain");

    list.push(1L, 1);
    list.push(2L, 2);
    check(list.size() == 2U, "size before clear");
    list.clear();
    check(list.empty(), "clear empties container");
    check(list.empty(), "size is 0 after clear");

    // Chrono timestamp coverage: verify chronological top_pop order.
    using steady_tp = std::chrono::steady_clock::time_point;
    tools::time_list<steady_tp, int> chrono_list;
    const auto base_time = std::chrono::steady_clock::now();

    chrono_list.push(base_time + std::chrono::milliseconds(300), 3);
    chrono_list.push(base_time + std::chrono::milliseconds(100), 1);
    chrono_list.push(base_time + std::chrono::milliseconds(200), 2);

    auto c1 = chrono_list.top_pop();
    auto c2 = chrono_list.top_pop();
    auto c3 = chrono_list.top_pop();

    check(c1.has_value() && c1->second == 1, "chrono top_pop order #1");
    check(c2.has_value() && c2->second == 2, "chrono top_pop order #2");
    check(c3.has_value() && c3->second == 3, "chrono top_pop order #3");

    std::cout << "  summary: passed=" << passed << " failed=" << failed << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_time_list()
{
    std::cout << "-- sync_time_list --" << '\n';

    tools::sync_time_list<long, int> sync_list;
    sync_list.push(30L, 30);
    sync_list.push(10L, 10);
    sync_list.push(40L, 40);
    sync_list.push(20L, 20);

    auto peeked = sync_list.top();
    if (peeked.has_value())
    {
        std::cout << "sync earliest: " << peeked->first << " => " << peeked->second << '\n';
    }

    auto snapshot = sync_list.snapshot_sorted();
    std::cout << "sync snapshot order:" << '\n';
    for (const auto& entry : snapshot)
    {
        std::cout << "  " << entry.first << " => " << entry.second << '\n';
    }

    std::cout << "sync drain order:" << '\n';
    while (!sync_list.empty())
    {
        auto entry = sync_list.top_pop();
        if (entry.has_value())
        {
            std::cout << "  " << entry->first << " => " << entry->second << '\n';
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_sync_time_list_unit_style()
{
    std::cout << "-- sync_time_list unit-style checks --" << '\n';

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

    tools::sync_time_list<long, int> list;

    check(list.empty(), "starts empty");
    check(list.empty(), "starts with size 0");
    check(!list.top().has_value(), "top() empty returns nullopt");
    check(!list.top_pop().has_value(), "top_pop() empty returns nullopt");

    list.push(30L, 30);
    list.push(10L, 10);
    list.push(40L, 40);
    list.emplace(20L, 20);

    check(!list.empty(), "non-empty after push/emplace");
    check(list.size() == 4U, "size after 4 inserts");

    auto top_entry = list.top();
    check(top_entry.has_value(), "top() returns value when non-empty");
    if (top_entry.has_value())
    {
        check(top_entry->first == 10L, "top() returns earliest timestamp");
        check(top_entry->second == 10, "top() returns expected payload");
    }

    auto snapshot = list.snapshot_sorted();
    check(snapshot.size() == 4U, "snapshot size matches");
    if (snapshot.size() == 4U)
    {
        check(snapshot[0].first == 10L, "snapshot[0] timestamp");
        check(snapshot[1].first == 20L, "snapshot[1] timestamp");
        check(snapshot[2].first == 30L, "snapshot[2] timestamp");
        check(snapshot[3].first == 40L, "snapshot[3] timestamp");
    }

    check(list.size() == 4U, "snapshot does not drain container");

    auto first = list.top_pop();
    auto second = list.top_pop();
    auto third = list.top_pop();
    auto fourth = list.top_pop();

    check(first.has_value() && first->first == 10L, "top_pop order #1");
    check(second.has_value() && second->first == 20L, "top_pop order #2");
    check(third.has_value() && third->first == 30L, "top_pop order #3");
    check(fourth.has_value() && fourth->first == 40L, "top_pop order #4");
    check(list.empty(), "empty after full drain");

    list.push(5L, 5);
    list.push(15L, 15);
    check(list.size() == 2U, "size before clear");
    list.clear();
    check(list.empty(), "clear empties container");
    check(list.empty(), "size is 0 after clear");

    std::cout << "  summary: passed=" << passed << " failed=" << failed << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------

void test_histogram()
{
    std::cout << "-- histogram --" << '\n';
    tools::histogram<double> hist;

    hist.add(1.0);
    hist.emplace(2.0);

    // add_range (C++17): iterator-pair batch insertion
    std::vector<double> samples = { 1.0, 2.0, 2.0, 3.5, 3.5, 3.5 };
    const auto inserted_pair = hist.add_range(samples.begin(), samples.end());
    std::cout << "add_range iterator-pair inserted: " << inserted_pair << '\n';

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
    // add_range (C++20): range overload with a container
    std::vector<double> extra = { -2.0, -1.0, 0.0, 0.5, 4.0, 5.0 };
    const auto inserted_range = hist.add_range(extra);
    std::cout << "add_range C++20 range inserted: " << inserted_range << '\n';

    // add_range (C++20): range overload with a filtered view
    auto non_negative = extra | std::views::filter([](double value) { return value >= 0.0; });
    const auto inserted_view = hist.add_range(non_negative);
    std::cout << "add_range C++20 filtered view inserted: " << inserted_view << '\n';
#endif

    const auto avg = hist.average();
    const auto var = hist.variance(avg);
    std::cout << "hist total count: " << hist.total_count() << '\n';
    std::cout << "hist top value: " << hist.top() << " (" << hist.top_occurence() << " times)" << '\n';
    std::cout << "hist avg: " << avg << " median: " << hist.median() << " variance: " << var << '\n';
}

//--------------------------------------------------------------------------------------------------------------------------------


void run_example_time_list()
{
    test_time_list();
    test_time_list_unit_style();
    test_sync_time_list();
    test_sync_time_list_unit_style();
    test_histogram();
}
