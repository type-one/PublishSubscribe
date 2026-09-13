/**
 * @file test_histogram.cpp
 * @brief Unit tests for the tools::histogram class template.
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

#include <memory>
#include <vector>

#include "tools/histogram.hpp"

template <typename T>
class HistogramTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        hist = std::make_unique<tools::histogram<T>>();
    }

    void TearDown() override
    {
        hist.reset();
    }

    std::unique_ptr<tools::histogram<T>> hist;
};

using MyTypes = ::testing::Types<int, float, double>;
TYPED_TEST_SUITE(HistogramTest, MyTypes);

TYPED_TEST(HistogramTest, AddAndTop)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));

    EXPECT_NEAR(static_cast<double>(this->hist->top()), 5.0, 1e-6);
    EXPECT_EQ(this->hist->total_count(), 3);
    EXPECT_EQ(this->hist->top_occurence(), 2);
}

TYPED_TEST(HistogramTest, Average)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));

    EXPECT_NEAR(this->hist->average(), 5.6666666666667, 1e-6);
}

TYPED_TEST(HistogramTest, Variance)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));

    const auto avg = this->hist->average();
    const auto variance = this->hist->variance(avg);

    EXPECT_NEAR(variance, 2.2222222222222, 1e-6);
    EXPECT_NEAR(this->hist->standard_deviation(variance), 1.4907119849999, 1e-6);
}

TYPED_TEST(HistogramTest, MedianEven)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));

    EXPECT_NEAR(this->hist->median(), 6.0, 1e-6);
}

TYPED_TEST(HistogramTest, MedianOdd)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(8));

    EXPECT_NEAR(this->hist->median(), 7.0, 1e-6);
}

TYPED_TEST(HistogramTest, GaussianDensity)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));

    const auto avg = this->hist->average();
    const auto variance = this->hist->variance(avg);
    const auto density
        = this->hist->gaussian_density(static_cast<TypeParam>(5), avg, this->hist->standard_deviation(variance));

    EXPECT_GT(density, 0.0);
}

TYPED_TEST(HistogramTest, GaussianProbability)
{
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(3));
    this->hist->add(static_cast<TypeParam>(5));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));
    this->hist->add(static_cast<TypeParam>(7));

    const auto avg = this->hist->average();
    const auto variance = this->hist->variance(avg);
    const auto prob = this->hist->gaussian_probability(
        static_cast<TypeParam>(3), static_cast<TypeParam>(5), avg, this->hist->standard_deviation(variance), 100);

    EXPECT_GT(prob, 0.0);
}

TYPED_TEST(HistogramTest, EmptyHistogram)
{
    EXPECT_EQ(this->hist->total_count(), 0);
    EXPECT_EQ(this->hist->top_occurence(), 0);
    EXPECT_NEAR(this->hist->average(), 0.0, 1e-6);
    EXPECT_NEAR(this->hist->variance(0.0), 0.0, 1e-6);
    EXPECT_NEAR(this->hist->median(), 0.0, 1e-6);
}

TEST(HistogramPerfectForwardingTest, AddSupportsExactAndConversionPaths)
{
    tools::histogram<double> hist;

    double lvalue_value = 2.5;
    hist.add(lvalue_value); // exact-T lvalue overload
    hist.add(2.5);          // exact-T rvalue overload
    hist.emplace(2);        // in-place construction from int

    EXPECT_EQ(hist.total_count(), 3);
    EXPECT_EQ(hist.top_occurence(), 2);
    EXPECT_NEAR(hist.top(), 2.5, 1e-6);
}

TEST(HistogramRangeTest, AddRangeSupportsRangeAndInitializerList)
{
    tools::histogram<double> hist;

    const std::vector<double> initial_values = { 2.5, 2.5 };
    hist.add_range(initial_values.begin(), initial_values.end());
    const std::vector<int> extra_values = { 2, 3 };
    hist.add_range(extra_values.begin(), extra_values.end());

    EXPECT_EQ(hist.total_count(), 4);
    EXPECT_EQ(hist.top_occurence(), 2);
    EXPECT_NEAR(hist.top(), 2.5, 1e-6);
}

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
TEST(HistogramRangeTest, AddRangeFromCpp20Range)
{
    tools::histogram<double> hist;
    const std::vector<int> values = { 1, 1, 2 };
    hist.add_range(values);

    EXPECT_EQ(hist.total_count(), 3);
    EXPECT_NEAR(hist.top(), 1.0, 1e-6);
}
#endif
