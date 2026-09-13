/**
 * @file test_lock_free_ring_buffer.cpp
 * @brief Unit tests for the tools::lock_free_ring_buffer class template.
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

#include <array>
#include <memory>
#include <thread>
#include <vector>

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
#include <span>
#endif

#include "tools/lock_free_ring_buffer.hpp"

template <typename T>
class LockFreeRingBufferTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        buffer = std::make_unique<tools::lock_free_ring_buffer<T, 4>>();
    }

    void TearDown() override
    {
        buffer.reset();
    }

    std::unique_ptr<tools::lock_free_ring_buffer<T, 4>> buffer;
};

using MyTypes = ::testing::Types<int, float, double, char>;
TYPED_TEST_SUITE(LockFreeRingBufferTest, MyTypes);

TYPED_TEST(LockFreeRingBufferTest, CapacityTest)
{
    ASSERT_EQ(this->buffer->capacity(), 16U);
}

// TODO: PublishSubscribeESP32's lock_free_ring_buffer wastes one slot to distinguish
// full/empty, so only capacity()-1 slots are usable there; this repo's version tracks
// size explicitly and all capacity() slots are usable.
TYPED_TEST(LockFreeRingBufferTest, PushPopTest)
{
    // capacity is a full power-of-two (no sentinel slot wasted), so all 16 slots are usable
    TypeParam value;
    for (int i = 1; i <= 16; ++i)
    {
        ASSERT_TRUE(this->buffer->push(static_cast<TypeParam>(i)));
    }
    ASSERT_FALSE(this->buffer->push(static_cast<TypeParam>(17))); // buffer should be full

    for (int i = 1; i <= 16; ++i)
    {
        ASSERT_TRUE(this->buffer->pop(value));
        ASSERT_EQ(value, static_cast<TypeParam>(i));
    }
    ASSERT_FALSE(this->buffer->pop(value)); // buffer should be empty
}

TYPED_TEST(LockFreeRingBufferTest, PushPopInterleavedTest)
{
    TypeParam value;
    ASSERT_TRUE(this->buffer->push(static_cast<TypeParam>(1)));
    ASSERT_TRUE(this->buffer->push(static_cast<TypeParam>(2)));
    ASSERT_TRUE(this->buffer->pop(value));
    ASSERT_EQ(value, static_cast<TypeParam>(1));
    ASSERT_TRUE(this->buffer->push(static_cast<TypeParam>(3)));
    ASSERT_TRUE(this->buffer->pop(value));
    ASSERT_EQ(value, static_cast<TypeParam>(2));
    ASSERT_TRUE(this->buffer->pop(value));
    ASSERT_EQ(value, static_cast<TypeParam>(3));
    ASSERT_FALSE(this->buffer->pop(value));
}

TYPED_TEST(LockFreeRingBufferTest, OverflowTest)
{
    for (int i = 0; i < 16; ++i)
    {
        ASSERT_TRUE(this->buffer->push(static_cast<TypeParam>(i)));
    }
    ASSERT_FALSE(this->buffer->push(static_cast<TypeParam>(16)));
}

TYPED_TEST(LockFreeRingBufferTest, UnderflowTest)
{
    TypeParam value;
    ASSERT_FALSE(this->buffer->pop(value));
}

TYPED_TEST(LockFreeRingBufferTest, ProducerConsumerInterleavedTest)
{
    std::thread producer(
        [this]()
        {
            for (int i = 0; i < 100000; ++i)
            {
                while (!this->buffer->push(static_cast<TypeParam>(i)))
                {
                    std::this_thread::yield();
                }
            }
        });

    std::thread consumer(
        [this]()
        {
            TypeParam value;
            for (int i = 0; i < 100000; ++i)
            {
                while (!this->buffer->pop(value))
                {
                    std::this_thread::yield();
                }
                ASSERT_EQ(value, static_cast<TypeParam>(i));
            }
        });

    producer.join();
    consumer.join();
}

TEST(LockFreeRingBufferRangeTest, PushRangeSupportsInitializerAndRange)
{
    tools::lock_free_ring_buffer<int, 3> buffer; // capacity 8

    const std::vector<int> initial_values = { 1, 2, 3 };
    const std::size_t pushed_init = buffer.push_range(initial_values.begin(), initial_values.end());
    ASSERT_EQ(pushed_init, 3U);

    const std::vector<int> extra_values = { 4, 5 };
    const std::size_t pushed_vec = buffer.push_range(extra_values.begin(), extra_values.end());
    ASSERT_EQ(pushed_vec, 2U);

    for (int expected = 1; expected <= 5; ++expected)
    {
        int value = 0;
        ASSERT_TRUE(buffer.pop(value));
        EXPECT_EQ(value, expected);
    }
    int trailing = 0;
    EXPECT_FALSE(buffer.pop(trailing));
}

TEST(LockFreeRingBufferRangeTest, PopRangeIteratorReturnsEffectiveCount)
{
    tools::lock_free_ring_buffer<int, 3> buffer;
    const std::vector<int> values = { 10, 20, 30, 40 };
    const std::size_t pushed = buffer.push_range(values.begin(), values.end());
    ASSERT_EQ(pushed, 4U);

    std::array<int, 3> destination = { 0, 0, 0 };
    const std::size_t popped_count = buffer.pop_range(destination.begin(), destination.end());

    ASSERT_EQ(popped_count, 3U);
    EXPECT_EQ(destination[0], 10);
    EXPECT_EQ(destination[1], 20);
    EXPECT_EQ(destination[2], 30);

    int remaining = 0;
    ASSERT_TRUE(buffer.pop(remaining));
    EXPECT_EQ(remaining, 40);
    EXPECT_FALSE(buffer.pop(remaining));
}

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
TEST(LockFreeRingBufferRangeTest, PushRangeFromCpp20RangeAndSpanPop)
{
    tools::lock_free_ring_buffer<int, 3> buffer;
    const std::vector<int> values = { 3, 4, 5 };
    const std::size_t pushed = buffer.push_range(values);
    ASSERT_EQ(pushed, 3U);

    std::array<int, 2> destination = { 0, 0 };
    const std::size_t popped_count = buffer.pop_range(std::span<int>(destination));

    ASSERT_EQ(popped_count, 2U);
    EXPECT_EQ(destination[0], 3);
    EXPECT_EQ(destination[1], 4);

    int remaining = 0;
    ASSERT_TRUE(buffer.pop(remaining));
    EXPECT_EQ(remaining, 5);
}
#endif
