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

#if !defined(__SYNC_OBSERVER_HPP__)
#define __SYNC_OBSERVER_HPP__

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "non_copyable.hpp"

namespace tools
{
    // https://juanchopanzacpp.wordpress.com/2013/02/24/simple-observer-pattern-implementation-c11/
    // http://www.codeproject.com/Articles/328365/Understanding-and-Implementing-Observer-Pattern

    template <typename Topic, typename Evt>
    class sync_observer : public non_copyable
    {
    public:
        sync_observer() = default;
        virtual ~sync_observer() { }

        virtual void inform(const Topic& topic, const Evt& event, const std::string& origin) = 0;
    };

    template <typename Topic, typename Evt>
    using sync_subscription = std::pair<Topic, std::shared_ptr<sync_observer<Topic, Evt>>>;

    template <typename Topic, typename Evt>
    class sync_subject
    {
    public:
        using sync_observer_shared_ptr = std::shared_ptr<sync_observer<Topic, Evt>>;

        sync_subject() = delete;
        sync_subject(const std::string& name)
            : m_name { name }
        {
        }

        virtual ~sync_subject() { }

        std::string name() const { return m_name; }

        void subscribe(const Topic& topic, sync_observer_shared_ptr observer)
        {
            std::lock_guard guard(m_mutex);
            m_subscribers.insert({ topic, observer });
        }

        void unsubscribe(const Topic& topic, sync_observer_shared_ptr observer)
        {
            std::lock_guard guard(m_mutex);

            for (auto [it, range_end] = m_subscribers.equal_range(topic); it != range_end; ++it)
            {
                if (it->second == observer)
                {
                    m_subscribers.erase(it);
                    break;
                }
            }
        }

        virtual void publish(const Topic& topic, const Evt& event)
        {
            std::vector<sync_observer_shared_ptr> to_inform;

            {
                std::lock_guard guard(m_mutex);

                for (auto [it, range_end] = m_subscribers.equal_range(topic); it != range_end; ++it)
                {
                    to_inform.push_back(it->second);
                }
            }

            for (auto& observer : to_inform)
            {
                observer->inform(topic, event, m_name);
            }
        }

    private:
        std::mutex m_mutex;
        std::multimap<Topic, sync_observer_shared_ptr> m_subscribers;
        std::string m_name;
    };

}

#endif //  __SYNC_OBSERVER_HPP__
