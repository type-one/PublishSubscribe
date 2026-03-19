/**
 * @file async_observer.hpp
 * @brief A header file for the async_observer class providing asynchronous observation capabilities.
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

#if !defined(ASYNC_OBSERVER_HPP_)
#define ASYNC_OBSERVER_HPP_

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>


#include "tools/sync_object.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_queue.hpp"

namespace tools
{
    /**
     * @brief A class that provides asynchronous observation capabilities.
     *
     * This class inherits from sync_observer and allows events to be queued and processed asynchronously.
     *
     * @tparam Topic The type of the topic associated with the events.
     * @tparam Evt The type of the event data.
     */
    template <typename Topic, typename Evt>
    class async_observer : public sync_observer<Topic, Evt> // NOLINT inherits indirectly from non copyable/non movable
    {
    public:
        using event_entry = std::tuple<Topic, Evt, std::string>;

        async_observer() = default;
        virtual ~async_observer() = default;

        void inform(const Topic& topic, const Evt& event, const std::string& origin) override
        {
            // Virtual observer API keeps const references; enqueue copies into async storage.
            enqueue_event(topic, event, origin);
            m_wakeable.signal();
        }

        // Perfect forwarding path for producers that can pass temporaries/movables directly.
#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L))
        // C++20: requires clause constrains forwarded arguments to tuple-constructible ones.
        template <typename TopicArg, typename EvtArg, typename OriginArg>
            requires std::is_constructible_v<Topic, TopicArg&&> && std::is_constructible_v<Evt, EvtArg&&>
            && std::is_constructible_v<std::string, OriginArg&&>
        void enqueue_event(TopicArg&& topic, EvtArg&& event, OriginArg&& origin)
        {
            m_evt_queue.emplace(
                std::forward<TopicArg>(topic), std::forward<EvtArg>(event), std::forward<OriginArg>(origin));
        }
#else
        // C++17: std::enable_if_t provides equivalent SFINAE constraints.
        template <typename TopicArg, typename EvtArg, typename OriginArg,
            typename = std::enable_if_t<std::is_constructible_v<Topic, TopicArg&&>
                && std::is_constructible_v<Evt, EvtArg&&> && std::is_constructible_v<std::string, OriginArg&&>>>
        void enqueue_event(TopicArg&& topic, EvtArg&& event, OriginArg&& origin)
        {
            m_evt_queue.emplace(
                std::forward<TopicArg>(topic), std::forward<EvtArg>(event), std::forward<OriginArg>(origin));
        }
#endif

        std::vector<event_entry> pop_all_events()
        {
            std::vector<event_entry> events;

            while (!m_evt_queue.empty())
            {
                auto item = m_evt_queue.front_pop();
                if (item.has_value())
                {
                    events.emplace_back(*item);
                }
            }

            return events;
        }

        std::optional<event_entry> pop_first_event()
        {
            std::optional<event_entry> entry;

            if (!m_evt_queue.empty())
            {
                entry = m_evt_queue.front_pop();
            }

            return entry;
        }

        std::optional<event_entry> pop_last_event()
        {
            std::optional<event_entry> entry;

            if (!m_evt_queue.empty())
            {
                entry = m_evt_queue.back();

                if (entry.has_value())
                {
                    while (!m_evt_queue.empty())
                    {
                        m_evt_queue.pop();
                    }
                }
            }

            return entry;
        }

        [[nodiscard]] bool has_events() const
        {
            return !m_evt_queue.empty();
        }

        [[nodiscard]] std::size_t number_of_events() const
        {
            return m_evt_queue.size();
        }

        void wait_for_events()
        {
            m_wakeable.wait_for_signal();
        }

        void wait_for_events(const std::chrono::duration<int, std::micro>& timeout)
        {
            m_wakeable.wait_for_signal(timeout);
        }

    private:
        sync_object m_wakeable;
        sync_queue<event_entry> m_evt_queue;
    };

}

#endif //  ASYNC_OBSERVER_HPP_
