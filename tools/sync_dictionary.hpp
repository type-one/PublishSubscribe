/**
 * @file sync_dictionary.hpp
 * @brief A thread-safe dictionary class.
 *
 * This file contains the definition of the sync_dictionary class, which provides
 * a thread-safe dictionary implementation using a mutex to protect access to the
 * internal dictionary. It supports adding, removing, and retrieving key-value pairs,
 * as well as checking the size and emptiness of the dictionary.
 *
 * @author Laurent Lardinois
 * @date January 2025
 */
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

#if !defined(SYNC_DICTIONARY_HPP_)
#define SYNC_DICTIONARY_HPP_

#include <cstddef>
#include <map>
#include <shared_mutex>
#include <optional>
#include <unordered_map>

#include "tools/non_copyable.hpp"

namespace tools
{
    /**
     * @brief A thread-safe dictionary class.
     *
     * This class provides a thread-safe dictionary implementation using a mutex
     * to protect access to the internal dictionary. It supports adding, removing,
     * and retrieving key-value pairs, as well as checking the size and emptiness
     * of the dictionary.
     *
     * @tparam K The type of the keys in the dictionary.
     * @tparam T The type of the values in the dictionary.
     */
    template <typename K, typename T>
    class sync_dictionary : public non_copyable // NOLINT inherits from non copyable/non movable
    {
    public:
        sync_dictionary() = default;
        ~sync_dictionary() = default;

        void add(const K& key, const T& value)
        {
            std::unique_lock guard(m_mutex);
            m_dictionary[key] = value;
        }

        void remove(const K& key)
        {
            std::unique_lock guard(m_mutex);
            m_dictionary.erase(key);
        }

        void add_collection(const std::map<K, T>& collection)
        {
            std::unique_lock guard(m_mutex);
            for (const auto& [key, value] : collection)
            {
                m_dictionary[key] = value;
            }
        }

        void add_collection(const std::unordered_map<K, T>& collection)
        {
            std::unique_lock guard(m_mutex);
            for (const auto& [key, value] : collection)
            {
                m_dictionary[key] = value;
            }
        }

        std::map<K, T> get_collection() const
        {
            std::shared_lock guard(m_mutex);
            auto snapshot = m_dictionary;
            return snapshot;
        }

        std::optional<T> find(const K& key) const
        {
            std::optional<T> result;
            std::shared_lock guard(m_mutex);
            const auto& itk = m_dictionary.find(key);
            if (m_dictionary.cend() != itk)
            {
                result = itk->second;
            }
            return result;
        }

        bool empty() const
        {
            std::shared_lock guard(m_mutex);
            return m_dictionary.empty();
        }

        std::size_t size() const
        {
            std::shared_lock guard(m_mutex);
            return m_dictionary.size();
        }

        void clear()
        {
            std::unique_lock guard(m_mutex);
            m_dictionary.clear();
        }

    private:
        std::map<K, T> m_dictionary;
        mutable std::shared_mutex m_mutex;
    };
}

#endif //  SYNC_DICTIONARY_HPP_
