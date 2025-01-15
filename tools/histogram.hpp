//-----------------------------------------------------------------------------//
// C++ Publish/Subscribe Pattern - Spare time development for fun              //
// (c) 2025 Laurent Lardinois https://be.linkedin.com/in/laurentlardinois      //
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

#pragma once

#if !defined(__HISTOGRAM_HPP__)
#define __HISTOGRAM_HPP__

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#include "non_copyable.hpp"

#ifndef M_TWO_PI
#define M_TWO_PI (2.0 * 3.14159265358979323846)
#endif

namespace tools
{
    template <typename T>
    class histogram : public non_copyable
    {
    public:
        histogram() = default;
        ~histogram() = default;

        void add(const T& value)
        {
            if (0 == m_occurences.count(value)) // first time
            {
                m_occurences.emplace(value, 1);

                if (0 == m_top_occurence)
                {
                    m_top_occurence = 1;
                    m_top_value = value;
                }
            }
            else // found twice at least
            {
                auto found = m_occurences.find(value);
                if (found != m_occurences.end())
                {
                    found->second += 1;

                    if (found->second > m_top_occurence)
                    {
                        m_top_occurence = found->second;
                        m_top_value = value;
                    }
                }
            }

            ++m_total_count;
        }

        constexpr T top() const { return m_top_value; }

        constexpr int total_count() const { return m_total_count; }

        constexpr int top_occurence() const { return m_top_occurence; }

        T average() const
        {
            double avg = 0.0;
            double total = 0.0;

            for (auto it = m_occurences.cbegin(); it != m_occurences.cend(); ++it)
            {
                if (it->second > 0) // occurence
                {
                    avg += static_cast<double>(it->second * it->first);
                    total += static_cast<double>(it->second);
                }
            }

            return (total > 0.0) ? static_cast<T>((avg / total) + 0.5) : static_cast<T>(0);
        }

        T variance(const T& average) const
        {
            double vari = 0.0;
            double total = 0.0;

            for (auto it = m_occurences.cbegin(); it != m_occurences.cend(); ++it)
            {
                if (it->second > 0) // occurence
                {
                    const double v = static_cast<double>(it->first);
                    vari += it->second * v * v;
                    total += static_cast<double>(it->second);
                }
            }

            return (total > 0.0) ? static_cast<T>((vari / total) - static_cast<double>(average * average) + 0.5) : static_cast<T>(0);
        }

        T median() const
        {
            std::vector<T> to_sort;

            for (auto it = m_occurences.cbegin(); it != m_occurences.cend(); ++it)
            {
                for (int i = 0; i < it->second; ++i) // occurence
                {
                    to_sort.emplace_back(it->first);
                }
            }

            if (to_sort.empty())
            {
                return static_cast<T>(0);
            }

            std::sort(to_sort.begin(), to_sort.end());

            return to_sort[to_sort.size() >> 1];
        }

        double gaussian_probability(const T& value, const T& average, const T& variance) const
        {
            // https://fr.wikipedia.org/wiki/Loi_normale
            if (variance > static_cast<T>(0))
            {
                const double sigma = std::sqrt(static_cast<double>(variance));
                const double e = static_cast<double>(value - average) / sigma;
                return std::exp(-0.5 * e * e) / (sigma * std::sqrt(M_TWO_PI));
            }
            else
            {
                return 0.0;
            }
        }

    private:
        std::unordered_map<T, int> m_occurences;
        int m_total_count = 0;
        int m_top_occurence = 0;
        T m_top_value = static_cast<T>(0);
    };
}

#endif //  __HISTOGRAM_HPP__
