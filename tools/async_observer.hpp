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

#if !defined(__ASYNC_OBSERVER_HPP__)
#define __ASYNC_OBSERVER_HPP__

#include <cstddef>
#include <optional>
#include <tuple>
#include <vector>

#include "tools/sync_object.hpp"
#include "tools/sync_observer.hpp"
#include "tools/sync_queue.hpp"

namespace tools
{

    template <typename Topic, typename Evt>
    class async_observer : public sync_observer<Topic, Evt>
    {
    public:
        async_observer() = default;
        virtual ~async_observer() { }

        virtual void inform(const Topic& topic, const Evt& event, const std::string& origin) override
        {
            m_evt_queue.push({ topic, event, origin });
            m_wakeable.signal();
        }

        using event_entry = std::tuple<Topic, Evt, std::string>;

        std::vector<event_entry> pop_all_events()
        {
            std::vector<event_entry> events;

            while (!m_evt_queue.empty())
            {
                events.emplace_back(m_evt_queue.front());
                m_evt_queue.pop();
            }

            return events;
        }

        std::optional<event_entry> pop_first_event()
        {
            std::optional<event_entry> entry;

            if (!m_evt_queue.empty())
            {
                entry = m_evt_queue.front();
                m_evt_queue.pop();
            }

            return entry;
        }

        std::optional<event_entry> pop_last_event()
        {
            std::optional<event_entry> entry;

            if (!m_evt_queue.empty())
            {
                entry = m_evt_queue.back();

                while (!m_evt_queue.empty())
                {
                    m_evt_queue.pop();
                }
            }

            return entry;
        }

        bool has_events() { return !m_evt_queue.empty(); }

        std::size_t number_of_events() { return m_evt_queue.size(); }

        void wait_for_events() { m_wakeable.wait_for_signal(); }

        void wait_for_events(const std::chrono::duration<int, std::micro>& timeout) { m_wakeable.wait_for_signal(timeout); }

    private:
        sync_object m_wakeable;
        sync_queue<std::tuple<Topic, Evt, std::string>> m_evt_queue;
    };

}

#endif //  __ASYNC_OBSERVER_HPP__
