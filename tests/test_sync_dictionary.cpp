/**
 * @file test_sync_dictionary.cpp
 * @brief Unit tests for the tools::sync_dictionary class template.
 *
 * @author Laurent Lardinois and Copilot
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

#include <gtest/gtest.h>

#include <atomic>
#include <complex>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <ranges>
#endif

#include "tools/sync_dictionary.hpp"

/**
 * @brief Test fixture for testing the tools::sync_dictionary class template with different key types.
 * @tparam T The type of the values stored in the dictionary.
 */
template <typename T>
class SyncDictionaryTest : public ::testing::Test
{
protected:
    std::unique_ptr<tools::sync_dictionary<int, T>> dict_int_key;
    std::unique_ptr<tools::sync_dictionary<std::string, T>> dict_string_key;

    void SetUp() override
    {
        dict_int_key = std::make_unique<tools::sync_dictionary<int, T>>();
        dict_string_key = std::make_unique<tools::sync_dictionary<std::string, T>>();
    }

    void TearDown() override
    {
        dict_int_key.reset();
        dict_string_key.reset();
    }
};

using MyTypes = ::testing::Types<int, float, double, char, std::complex<double>>;
TYPED_TEST_SUITE(SyncDictionaryTest, MyTypes);

TYPED_TEST(SyncDictionaryTest, AddAndFindIntKey)
{
    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    this->dict_int_key->add(2, static_cast<TypeParam>(2));

    EXPECT_TRUE(this->dict_int_key->find(1).has_value());
    EXPECT_EQ(this->dict_int_key->find(1).value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_int_key->find(2).has_value());
    EXPECT_EQ(this->dict_int_key->find(2).value(), static_cast<TypeParam>(2));
    EXPECT_FALSE(this->dict_int_key->find(3).has_value());
}

TYPED_TEST(SyncDictionaryTest, AddAndFindStringKey)
{
    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    this->dict_string_key->add("two", static_cast<TypeParam>(2));

    EXPECT_TRUE(this->dict_string_key->find("one").has_value());
    EXPECT_EQ(this->dict_string_key->find("one").value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_string_key->find("two").has_value());
    EXPECT_EQ(this->dict_string_key->find("two").value(), static_cast<TypeParam>(2));
    EXPECT_FALSE(this->dict_string_key->find("three").has_value());
}

TYPED_TEST(SyncDictionaryTest, RemoveIntKey)
{
    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    this->dict_int_key->add(2, static_cast<TypeParam>(2));
    this->dict_int_key->remove(1);

    EXPECT_FALSE(this->dict_int_key->find(1).has_value());
    EXPECT_TRUE(this->dict_int_key->find(2).has_value());
}

TYPED_TEST(SyncDictionaryTest, RemoveStringKey)
{
    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    this->dict_string_key->add("two", static_cast<TypeParam>(2));
    this->dict_string_key->remove("one");

    EXPECT_FALSE(this->dict_string_key->find("one").has_value());
    EXPECT_TRUE(this->dict_string_key->find("two").has_value());
}

TYPED_TEST(SyncDictionaryTest, AddCollectionIntKey)
{
    std::map<int, TypeParam> collection = { { 1, static_cast<TypeParam>(1) }, { 2, static_cast<TypeParam>(2) } };
    this->dict_int_key->add_collection(collection);

    EXPECT_TRUE(this->dict_int_key->find(1).has_value());
    EXPECT_EQ(this->dict_int_key->find(1).value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_int_key->find(2).has_value());
    EXPECT_EQ(this->dict_int_key->find(2).value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, AddCollectionStringKey)
{
    std::map<std::string, TypeParam> collection
        = { { "one", static_cast<TypeParam>(1) }, { "two", static_cast<TypeParam>(2) } };
    this->dict_string_key->add_collection(collection);

    EXPECT_TRUE(this->dict_string_key->find("one").has_value());
    EXPECT_EQ(this->dict_string_key->find("one").value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_string_key->find("two").has_value());
    EXPECT_EQ(this->dict_string_key->find("two").value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, GetCollectionIntKey)
{
    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    this->dict_int_key->add(2, static_cast<TypeParam>(2));

    auto collection = this->dict_int_key->snapshot();
    EXPECT_EQ(collection.size(), 2U);
    EXPECT_EQ(collection[1], static_cast<TypeParam>(1));
    EXPECT_EQ(collection[2], static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, GetCollectionStringKey)
{
    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    this->dict_string_key->add("two", static_cast<TypeParam>(2));

    auto collection = this->dict_string_key->snapshot();
    EXPECT_EQ(collection.size(), 2U);
    EXPECT_EQ(collection["one"], static_cast<TypeParam>(1));
    EXPECT_EQ(collection["two"], static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, EmptyAndSizeIntKey)
{
    EXPECT_TRUE(this->dict_int_key->empty());
    EXPECT_EQ(this->dict_int_key->size(), 0U);

    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    EXPECT_FALSE(this->dict_int_key->empty());
    EXPECT_EQ(this->dict_int_key->size(), 1U);
}

TYPED_TEST(SyncDictionaryTest, EmptyAndSizeStringKey)
{
    EXPECT_TRUE(this->dict_string_key->empty());
    EXPECT_EQ(this->dict_string_key->size(), 0U);

    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    EXPECT_FALSE(this->dict_string_key->empty());
    EXPECT_EQ(this->dict_string_key->size(), 1U);
}

TYPED_TEST(SyncDictionaryTest, ClearIntKey)
{
    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    this->dict_int_key->add(2, static_cast<TypeParam>(2));
    this->dict_int_key->clear();

    EXPECT_TRUE(this->dict_int_key->empty());
    EXPECT_EQ(this->dict_int_key->size(), 0U);
}

TYPED_TEST(SyncDictionaryTest, ClearStringKey)
{
    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    this->dict_string_key->add("two", static_cast<TypeParam>(2));
    this->dict_string_key->clear();

    EXPECT_TRUE(this->dict_string_key->empty());
    EXPECT_EQ(this->dict_string_key->size(), 0U);
}

TYPED_TEST(SyncDictionaryTest, AddDuplicateKeyIntKey)
{
    this->dict_int_key->add(1, static_cast<TypeParam>(1));
    this->dict_int_key->add(1, static_cast<TypeParam>(2));

    EXPECT_TRUE(this->dict_int_key->find(1).has_value());
    EXPECT_EQ(this->dict_int_key->find(1).value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, AddDuplicateKeyStringKey)
{
    this->dict_string_key->add("one", static_cast<TypeParam>(1));
    this->dict_string_key->add("one", static_cast<TypeParam>(2));

    EXPECT_TRUE(this->dict_string_key->find("one").has_value());
    EXPECT_EQ(this->dict_string_key->find("one").value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, AddUnorderedCollectionIntKey)
{
    std::unordered_map<int, TypeParam> collection
        = { { 1, static_cast<TypeParam>(1) }, { 2, static_cast<TypeParam>(2) } };
    this->dict_int_key->add_collection(collection);

    EXPECT_TRUE(this->dict_int_key->find(1).has_value());
    EXPECT_EQ(this->dict_int_key->find(1).value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_int_key->find(2).has_value());
    EXPECT_EQ(this->dict_int_key->find(2).value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, AddUnorderedCollectionStringKey)
{
    std::unordered_map<std::string, TypeParam> collection
        = { { "one", static_cast<TypeParam>(1) }, { "two", static_cast<TypeParam>(2) } };
    this->dict_string_key->add_collection(collection);

    EXPECT_TRUE(this->dict_string_key->find("one").has_value());
    EXPECT_EQ(this->dict_string_key->find("one").value(), static_cast<TypeParam>(1));
    EXPECT_TRUE(this->dict_string_key->find("two").has_value());
    EXPECT_EQ(this->dict_string_key->find("two").value(), static_cast<TypeParam>(2));
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndFindIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                while (count.load() <= i)
                {
                    std::this_thread::yield();
                }
                EXPECT_TRUE(this->dict_int_key->find(i).has_value());
            }
        });

    t1.join();
    t2.join();
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndRemoveIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                while (count.load() <= i)
                {
                    std::this_thread::yield();
                }
                this->dict_int_key->remove(i);
            }
        });

    t1.join();
    t2.join();

    EXPECT_TRUE(this->dict_int_key->empty());
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndClearIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 100)
            {
                std::this_thread::yield();
            }
            this->dict_int_key->clear();
        });

    t1.join();
    t2.join();

    EXPECT_TRUE(this->dict_int_key->empty());
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndGetCollectionIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 100)
            {
                std::this_thread::yield();
            }
            auto collection = this->dict_int_key->snapshot();
            EXPECT_EQ(collection.size(), 100U);
        });

    t1.join();
    t2.join();
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndSizeIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 100)
            {
                std::this_thread::yield();
            }
            EXPECT_EQ(this->dict_int_key->size(), 100U);
        });

    t1.join();
    t2.join();
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddAndEmptyIntKey)
{
    std::atomic<int> count { 0 };

    std::thread t1(
        [this, &count]()
        {
            for (int i = 0; i < 100; ++i)
            {
                this->dict_int_key->add(i, static_cast<TypeParam>(i));
                count.fetch_add(1);
            }
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 100)
            {
                std::this_thread::yield();
            }
            EXPECT_FALSE(this->dict_int_key->empty());
        });

    t1.join();
    t2.join();
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddCollectionAndFindIntKey)
{
    std::atomic<int> count { 0 };

    std::map<int, TypeParam> collection;
    for (int i = 0; i < 100; ++i)
    {
        collection[i] = static_cast<TypeParam>(i);
    }

    std::thread t1(
        [this, &collection, &count]()
        {
            this->dict_int_key->add_collection(collection);
            count.fetch_add(1);
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 1)
            {
                std::this_thread::yield();
            }

            for (int i = 0; i < 100; ++i)
            {
                EXPECT_TRUE(this->dict_int_key->find(i).has_value());
            }
        });

    t1.join();
    t2.join();
}

TYPED_TEST(SyncDictionaryTest, ConcurrentAddUnorderedCollectionAndFindIntKey)
{
    std::atomic<int> count { 0 };

    std::unordered_map<int, TypeParam> collection;
    for (int i = 0; i < 100; ++i)
    {
        collection[i] = static_cast<TypeParam>(i);
    }

    std::thread t1(
        [this, &collection, &count]()
        {
            this->dict_int_key->add_collection(collection);
            count.fetch_add(1);
        });

    std::thread t2(
        [this, &count]()
        {
            while (count.load() < 1)
            {
                std::this_thread::yield();
            }

            for (int i = 0; i < 100; ++i)
            {
                EXPECT_TRUE(this->dict_int_key->find(i).has_value());
            }
        });

    t1.join();
    t2.join();
}

/**
 * @brief Verifies sync_dictionary add overloads support exact and conversion call paths.
 */
TEST(SyncDictionaryPerfectForwardingTest, AddSupportsExactAndConversionPaths)
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    std::string key_lvalue = "lvalue-key";
    std::string value_lvalue = "lvalue-value";

    str_dict.add(key_lvalue, value_lvalue);
    str_dict.add(std::string("rvalue-key"), std::string("rvalue-value"));
    str_dict.add("conversion-key", "conversion-value");
    str_dict.add("conversion-key", std::string("conversion-updated"));

    auto lvalue_found = str_dict.find("lvalue-key");
    auto rvalue_found = str_dict.find("rvalue-key");
    auto conversion_found = str_dict.find("conversion-key");

    ASSERT_TRUE(lvalue_found.has_value());
    ASSERT_TRUE(rvalue_found.has_value());
    ASSERT_TRUE(conversion_found.has_value());

    EXPECT_EQ(lvalue_found.value(), "lvalue-value");
    EXPECT_EQ(rvalue_found.value(), "rvalue-value");
    EXPECT_EQ(conversion_found.value(), "conversion-updated");
}

/**
 * @brief Verifies the add_emplace in-place construction overload.
 */
TEST(SyncDictionaryPerfectForwardingTest, AddEmplaceConstructsInPlace)
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    str_dict.add_emplace(std::make_tuple("emplaced-key"), std::make_tuple("emplaced-value"));

    auto found = str_dict.find("emplaced-key");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), "emplaced-value");
}

/**
 * @brief Verifies iterator-pair and C++20 range-based add_range, plus add_collection.
 */
TEST(SyncDictionaryRangeTest, AddRangeAndAddCollection)
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    const std::vector<std::pair<std::string, std::string>> entries = { { "alpha", "one" }, { "beta", "two" } };

    const auto inserted = str_dict.add_range(entries.begin(), entries.end());
    EXPECT_EQ(inserted, 2U);

    str_dict.add_collection({ { "gamma", "three" }, { "beta", "two-updated" } });

    EXPECT_TRUE(str_dict.contains("alpha"));
    EXPECT_TRUE(str_dict.contains("beta"));
    EXPECT_TRUE(str_dict.contains("gamma"));

    auto beta_value = str_dict.find("beta");
    ASSERT_TRUE(beta_value.has_value());
    EXPECT_EQ(beta_value.value(), "two-updated");
}

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
/**
 * @brief Verifies the C++20 range overload of add_range with a filtered view.
 */
TEST(SyncDictionaryRangeTest, AddRangeFromFilteredView)
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    std::vector<std::pair<std::string, std::string>> entries
        = { { "keep_a", "va" }, { "drop_a", "vb" }, { "keep_b", "vc" } };

    auto filtered = entries | std::views::filter([](const auto& kv) { return kv.first.starts_with("keep"); });
    const auto inserted = str_dict.add_range(filtered);

    EXPECT_EQ(inserted, 2U);
    EXPECT_TRUE(str_dict.contains("keep_a"));
    EXPECT_TRUE(str_dict.contains("keep_b"));
    EXPECT_FALSE(str_dict.contains("drop_a"));
}
#endif

/**
 * @brief Verifies the internal associative container can be configured to std::unordered_map.
 */
TEST(SyncDictionaryContainerTypeTest, SupportsConfiguredUnorderedMap)
{
    using dict_t = tools::sync_dictionary<int, std::string, std::unordered_map<int, std::string>>;

    dict_t dictionary;
    dictionary.add(1, "one");
    dictionary.add(2, "two");

    auto snapshot = dictionary.snapshot();
    static_assert(std::is_same<decltype(snapshot), std::unordered_map<int, std::string>>::value,
        "snapshot type must follow configured dictionary container");

    ASSERT_EQ(snapshot.size(), 2U);
    ASSERT_TRUE(dictionary.contains(1));
    ASSERT_TRUE(dictionary.contains(2));
}

/**
 * @brief Tests std::string_view key arguments for add, find, contains, and remove operations.
 */
TEST(SyncDictionaryStringViewTest, AddFindContainsRemoveWithStringViewKeys)
{
    tools::sync_dictionary<std::string, std::string> str_dict;

    const std::string_view key_sv_1 = "key_one";
    const std::string_view val_sv_1 = "val_one";
    const std::string_view key_sv_2 = "key_two";
    const std::string_view val_sv_2 = "val_two";

    str_dict.add(key_sv_1, val_sv_1);
    str_dict.add(key_sv_2, val_sv_2);

    EXPECT_TRUE(str_dict.contains(key_sv_1));
    EXPECT_TRUE(str_dict.contains(key_sv_2));
    EXPECT_FALSE(str_dict.contains(std::string_view("key_three")));

    const auto found_val_1 = str_dict.find(key_sv_1);
    const auto found_val_2 = str_dict.find(key_sv_2);

    ASSERT_TRUE(found_val_1.has_value());
    ASSERT_TRUE(found_val_2.has_value());
    EXPECT_EQ(found_val_1.value(), "val_one");
    EXPECT_EQ(found_val_2.value(), "val_two");

    str_dict.remove(key_sv_1);
    EXPECT_FALSE(str_dict.contains(key_sv_1));
    EXPECT_TRUE(str_dict.contains(key_sv_2));
}

/**
 * @brief Tests std::string_view key operations with transparent comparator std::less<>.
 */
TEST(SyncDictionaryStringViewTest, TransparentLookupWithLess)
{
    using transparent_dict_t = tools::sync_dictionary<std::string, int, std::map<std::string, int, std::less<>>>;
    transparent_dict_t dict;

    const std::string_view alpha_key = "alpha";
    const std::string_view beta_key = "beta";

    dict.add(alpha_key, 100);
    dict.add(beta_key, 200);

    EXPECT_TRUE(dict.contains(alpha_key));
    EXPECT_TRUE(dict.contains(beta_key));

    const auto alpha_val = dict.find(alpha_key);
    ASSERT_TRUE(alpha_val.has_value());
    EXPECT_EQ(alpha_val.value(), 100);

    dict.remove(alpha_key);
    EXPECT_FALSE(dict.contains(alpha_key));
    EXPECT_TRUE(dict.contains(beta_key));
}

/**
 * @brief Tests std::string_view key operations with std::unordered_map container.
 */
TEST(SyncDictionaryStringViewTest, UnorderedMapWithStringViewKeys)
{
    using dict_t = tools::sync_dictionary<std::string, std::string, std::unordered_map<std::string, std::string>>;
    dict_t dict;

    const std::string_view key_sv = "unordered_key";
    const std::string_view val_sv = "unordered_val";

    dict.add(key_sv, val_sv);

    EXPECT_TRUE(dict.contains(key_sv));
    const auto found = dict.find(key_sv);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), "unordered_val");

    dict.remove(key_sv);
    EXPECT_FALSE(dict.contains(key_sv));
}

/**
 * @brief Tests sync_dictionary when std::string_view is used as the native key type K.
 */
TEST(SyncDictionaryStringViewTest, StringViewAsNativeKey)
{
    tools::sync_dictionary<std::string_view, int> dict;

    const std::string_view key_1 = "item_1";
    const std::string_view key_2 = "item_2";

    dict.add(key_1, 10);
    dict.add(key_2, 20);

    EXPECT_TRUE(dict.contains(key_1));
    EXPECT_TRUE(dict.contains(std::string("item_2")));
    EXPECT_TRUE(dict.contains("item_1"));

    const auto val_1 = dict.find(key_1);
    ASSERT_TRUE(val_1.has_value());
    EXPECT_EQ(val_1.value(), 10);

    dict.remove("item_1");
    EXPECT_FALSE(dict.contains(key_1));
}
