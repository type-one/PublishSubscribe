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

#if !defined(__SYNC_DICTIONARY_HPP__)
#define __SYNC_DICTIONARY_HPP__

#include <cstddef>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "non_copyable.hpp"

namespace tools
{
    template <typename K, typename T>
    class sync_dictionary : public non_copyable
    {
    public:
        sync_dictionary() = default;
        ~sync_dictionary() = default;

        void add(const K& key, const T& value)
        {
            std::lock_guard guard(m_mutex);
            m_dictionary[key] = value;
        }

        void remove(const K& key)
        {
            std::lock_guard guard(m_mutex);
            m_dictionary.erase(key);
        }

        void add_collection(const std::map<K, T> collection)
        {
            std::lock_guard guard(m_mutex);
            for (const auto& [key, value] : collection)
            {
                m_dictionary[key] = value;
            }
        }

        void add_collection(const std::unordered_map<K, T> collection)
        {
            std::lock_guard guard(m_mutex);
            for (const auto& [key, value] : collection)
            {
                m_dictionary[key] = value;
            }
        }

        std::map<K, T> get_collection()
        {
            std::lock_guard guard(m_mutex);
            auto snapshot = m_dictionary;
            return snapshot;
        }

        std::optional<T> find(const K& key)
        {
            std::optional<T> result;
            std::lock_guard guard(m_mutex);
            const auto& it = m_dictionary.find(key);
            if (m_dictionary.cend() != it)
            {
                result = it->second;
            }
            return result;
        }

        bool empty()
        {
            std::lock_guard guard(m_mutex);
            return m_dictionary.empty();
        }

        std::size_t size()
        {
            std::lock_guard guard(m_mutex);
            return m_dictionary.size();
        }

        void clear()
        {
            std::lock_guard guard(m_mutex);
            m_dictionary.clear();
        }

    private:
        std::map<K, T> m_dictionary;
        std::mutex m_mutex;
    };
}

#endif //  __SYNC_DICTIONARY_HPP__
