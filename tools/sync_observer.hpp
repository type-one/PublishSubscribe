/**
 * @file sync_observer.hpp
 * @brief Header file for the synchronous observer pattern implementation.
 *
 * This file contains the definition of the sync_observer and sync_subject classes,
 * which implement a synchronous observer pattern for event handling.
 *
 * @date January 2025
 * @author Laurent Lardinois
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

#if !defined(SYNC_OBSERVER_HPP_)
#define SYNC_OBSERVER_HPP_

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <utility>

#include "tools/non_copyable.hpp"

namespace tools
{
    // https://juanchopanzacpp.wordpress.com/2013/02/24/simple-observer-pattern-implementation-c11/
    // http://www.codeproject.com/Articles/328365/Understanding-and-Implementing-Observer-Pattern

    template <typename Topic, typename Evt>
    class sync_observer : public non_copyable // NOLINT inherits from non copyable/non movable
    {
    public:
        sync_observer() = default;
        virtual ~sync_observer() = default;

        virtual void inform(const Topic& topic, const Evt& event, const std::string& origin) = 0;
    };

    template <typename Topic, typename Evt>
    using sync_subscription = std::pair<Topic, std::shared_ptr<sync_observer<Topic, Evt>>>;

    template <typename Topic, typename Evt>
    using loose_coupled_handler = std::function<void(const Topic&, const Evt&, const std::string&)>;

    template <typename Topic, typename Evt>
    class sync_subject : public non_copyable // NOLINT inherits from non copyable and non movable class
    {
    public:
        using sync_observer_shared_ptr = std::shared_ptr<sync_observer<Topic, Evt>>;
        using handler = loose_coupled_handler<Topic, Evt>;

        sync_subject() = delete;
        sync_subject(const std::string& name)
            : m_name { name }
        {
        }

        virtual ~sync_subject() = default;

        [[nodiscard]] std::string name() const
        {
            return m_name;
        }

        void subscribe(const Topic& topic, sync_observer_shared_ptr observer)
        {
            std::unique_lock guard(m_mutex);
            m_subscribers.insert({ topic, observer });
        }

        void subscribe(const Topic& topic, const std::string& handler_name, handler handler)
        {
            std::unique_lock guard(m_mutex);
            m_handlers.insert({ topic, std::make_pair(handler_name, handler) });
        }

        void unsubscribe(const Topic& topic, sync_observer_shared_ptr observer)
        {
            std::unique_lock guard(m_mutex);

            for (auto [it, range_end] = m_subscribers.equal_range(topic); it != range_end; ++it)
            {
                if (it->second == observer)
                {
                    m_subscribers.erase(it);
                    break;
                }
            }
        }

        void unsubscribe(const Topic& topic, const std::string& handler_name)
        {
            std::unique_lock guard(m_mutex);

            for (auto [it, range_end] = m_handlers.equal_range(topic); it != range_end; ++it)
            {
                if (it->second.first == handler_name)
                {
                    m_handlers.erase(it);
                    break;
                }
            }
        }

        virtual void publish(const Topic& topic, const Evt& event) const
        {
            std::vector<sync_observer_shared_ptr> to_inform;
            std::vector<handler> to_invoke;

            {
                std::shared_lock guard(m_mutex);

                for (auto [it, range_end] = m_subscribers.equal_range(topic); it != range_end; ++it)
                {
                    to_inform.push_back(it->second);
                }

                for (auto [it, range_end] = m_handlers.equal_range(topic); it != range_end; ++it)
                {
                    to_invoke.emplace_back(it->second.second);
                }
            }

            for (auto& observer : to_inform)
            {
                observer->inform(topic, event, m_name);
            }

            for (auto& handler : to_invoke)
            {
                handler(topic, event, m_name);
            }
        }

    private:
        mutable std::shared_mutex m_mutex;
        std::multimap<Topic, sync_observer_shared_ptr> m_subscribers = {};
        std::multimap<Topic, std::pair<std::string, handler>> m_handlers = {};
        std::string m_name;
    };

}

#endif //  SYNC_OBSERVER_HPP_
