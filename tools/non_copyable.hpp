/**
 * @file non_copyable.hpp
 * @brief Defines a non-copyable and non-movable class.
 *
 * This header file contains the definition of the `non_copyable` class, which
 * prevents copying and moving of its instances. This is useful for classes
 * that manage resources that should not be duplicated or transferred.
 *
 * @author Laurent Lardinois
 * @date January 2025
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

#pragma once

#if !defined(NON_COPYABLE_HPP_)
#define NON_COPYABLE_HPP_

namespace tools
{
    class non_copyable
    {
    public:
        non_copyable() = default;
        ~non_copyable() = default;

        non_copyable(const non_copyable&) = delete;
        non_copyable(non_copyable&&) noexcept = delete;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable& operator=(non_copyable&&) noexcept = delete;
    };
}

#endif //  NON_COPYABLE_HPP_
