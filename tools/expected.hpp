/**
 * @file expected.hpp
 * @brief Expected/unexpected compatibility layer.
 *
 * This header exposes tools::expected, tools::unexpected, and tools::unexpect
 * with an API aligned to std::expected.
 *
 * - On C++23 with <expected> support, these names alias the standard library.
 * - Otherwise, a lightweight fallback implementation is provided.
 *
 * @author Laurent Lardinois
 * @date May 2026
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

#if !defined(TOOLS_EXPECTED_HPP_)
#define TOOLS_EXPECTED_HPP_

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

#if defined(__has_include)
#if __has_include(<expected>)
#include <expected>
#if defined(__cpp_lib_expected) && (__cpp_lib_expected >= 202202L)
#define TOOLS_HAS_STD_EXPECTED 1
#endif
#endif
#endif

namespace tools
{

#if defined(TOOLS_HAS_STD_EXPECTED)

    template <typename T, typename E>
    using expected = std::expected<T, E>;

    template <typename E>
    using unexpected = std::unexpected<E>;

    using unexpect_t = std::unexpect_t;
    inline constexpr unexpect_t unexpect = std::unexpect;

#else

    template <typename E>
    class unexpected
    {
    private:
        E m_error;

    public:
        explicit unexpected(const E& error_value)
            : m_error(error_value)
        {
        }

        explicit unexpected(E&& error_value)
            : m_error(std::move(error_value))
        {
        }

        E& error() &
        {
            return m_error;
        }

        [[nodiscard]] const E& error() const&
        {
            return m_error;
        }

        E&& error() &&
        {
            return std::move(m_error);
        }

        [[nodiscard]] const E&& error() const&&
        {
            return std::move(m_error);
        }
    };

    template <typename E>
    unexpected(E) -> unexpected<std::decay_t<E>>;

    struct unexpect_t
    {
        explicit constexpr unexpect_t() = default;
    };

    inline constexpr unexpect_t unexpect {};

    template <typename T, typename E>
    class expected
    {
    private:
        static_assert(!std::is_void<T>::value, "Use expected<void, E> specialization for void value type");

        static constexpr std::size_t kValueIndex = 0U;
        static constexpr std::size_t kErrorIndex = 1U;
        std::variant<T, E> m_data;

    public:
        expected(const T& value)
            : m_data(std::in_place_index<kValueIndex>, value)
        {
        }

        expected(T&& value)
            : m_data(std::in_place_index<kValueIndex>, std::move(value))
        {
        }

        expected(const unexpected<E>& error_result)
            : m_data(std::in_place_index<kErrorIndex>, error_result.error())
        {
        }

        expected(unexpected<E>&& error_result)
            : m_data(std::in_place_index<kErrorIndex>, std::move(error_result).error())
        {
        }

        template <typename... Args>
        explicit expected(unexpect_t unexpect_tag, Args&&... error_args)
            : m_data(std::in_place_index<kErrorIndex>, std::forward<Args>(error_args)...)
        {
            static_cast<void>(unexpect_tag);
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return m_data.index() == kValueIndex;
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        T& value() &
        {
            assert(has_value());
            return std::get<kValueIndex>(m_data);
        }

        [[nodiscard]] const T& value() const&
        {
            assert(has_value());
            return std::get<kValueIndex>(m_data);
        }

        T&& value() &&
        {
            assert(has_value());
            return std::move(std::get<kValueIndex>(m_data));
        }

        [[nodiscard]] const T&& value() const&&
        {
            assert(has_value());
            return std::move(std::get<kValueIndex>(m_data));
        }

        E& error() &
        {
            assert(!has_value());
            return std::get<kErrorIndex>(m_data);
        }

        [[nodiscard]] const E& error() const&
        {
            assert(!has_value());
            return std::get<kErrorIndex>(m_data);
        }

        E&& error() &&
        {
            assert(!has_value());
            return std::move(std::get<kErrorIndex>(m_data));
        }

        [[nodiscard]] const E&& error() const&&
        {
            assert(!has_value());
            return std::move(std::get<kErrorIndex>(m_data));
        }

        T* operator->()
        {
            return &value();
        }

        const T* operator->() const
        {
            return &value();
        }

        T& operator*() &
        {
            return value();
        }

        const T& operator*() const&
        {
            return value();
        }

        T&& operator*() &&
        {
            return std::move(value());
        }

        const T&& operator*() const&&
        {
            return std::move(value());
        }

        template <typename U>
        T value_or(U&& default_value) const&
        {
            return has_value() ? std::get<kValueIndex>(m_data) : static_cast<T>(std::forward<U>(default_value));
        }

        template <typename U>
        T value_or(U&& default_value) &&
        {
            return has_value() ? std::move(std::get<kValueIndex>(m_data))
                               : static_cast<T>(std::forward<U>(default_value));
        }
    };

    template <typename E>
    class expected<void, E>
    {
    private:
        static constexpr std::size_t kStatusIndex = 0U;
        static constexpr std::size_t kErrorIndex = 1U;
        std::variant<std::monostate, E> m_data;

    public:
        expected()
            : m_data(std::in_place_index<kStatusIndex>)
        {
        }

        expected(const unexpected<E>& error_result)
            : m_data(std::in_place_index<kErrorIndex>, error_result.error())
        {
        }

        expected(unexpected<E>&& error_result)
            : m_data(std::in_place_index<kErrorIndex>, std::move(error_result).error())
        {
        }

        template <typename... Args>
        explicit expected(unexpect_t unexpect_tag, Args&&... error_args)
            : m_data(std::in_place_index<kErrorIndex>, std::forward<Args>(error_args)...)
        {
            static_cast<void>(unexpect_tag);
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return m_data.index() == kStatusIndex;
        }

        explicit operator bool() const noexcept
        {
            return has_value();
        }

        void value() const
        {
            assert(has_value());
        }

        E& error() &
        {
            assert(!has_value());
            return std::get<kErrorIndex>(m_data);
        }

        [[nodiscard]] const E& error() const&
        {
            assert(!has_value());
            return std::get<kErrorIndex>(m_data);
        }

        E&& error() &&
        {
            assert(!has_value());
            return std::move(std::get<kErrorIndex>(m_data));
        }

        [[nodiscard]] const E&& error() const&&
        {
            assert(!has_value());
            return std::move(std::get<kErrorIndex>(m_data));
        }
    };

#endif

} // namespace tools

#endif // TOOLS_EXPECTED_HPP_
