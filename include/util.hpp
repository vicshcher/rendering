#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>
#include <source_location>
#include <sstream>
#include <type_traits>
#include <utility>

#define IGNORE(expr) static_cast<void>(expr);
#define _STR(str) #str
#define STR(str) _STR(str)
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

#define _MAKE_STREAM(stream)                                      \
    template<typename Char, typename...Args>                      \
    auto make_##stream(Args&&...args)                             \
    {                                                             \
        if constexpr (std::is_same_v<Char, char>) {               \
            return std::stream{ std::forward<Args>(args)... };    \
        }                                                         \
        else if (std::is_same_v<Char, wchar_t>) {                 \
            return std::w##stream{ std::forward<Args>(args)... }; \
        }                                                         \
        std::unreachable();                                       \
    }                                                             \

namespace util {

template<typename Enum>
    requires std::is_enum_v<Enum>
auto to_underlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e); // todo check
}

template<typename Enum>
    requires std::is_enum_v<Enum>
struct enum_type
{
    using Underlying = std::underlying_type_t<Enum>;

    constexpr enum_type(const enum_type& e) : value{e.value} {}
    constexpr enum_type(Enum e)             : value{std::to_underlying(e)} {}
    constexpr enum_type(Underlying e)       : value{e} {} // todo add check

    constexpr bool operator&(const enum_type& e)
    {
        return value & e.value;
    }

    constexpr bool operator&(const Underlying& e) const
    {
        return value & e;
    }

    constexpr bool operator|=(const enum_type& e)
    {
        return value |= e.value;
    }

    constexpr bool operator|=(const Underlying& e) 
    {
        return value |= e;
    }

    constexpr operator Enum() const
    {
        return Enum(value);
    }

    constexpr operator Underlying() const
    {
        return value;
    }

    constexpr auto operator<=>(const enum_type&) const = default;

    Underlying value;
};

template<typename Enum>
    requires std::is_enum_v<Enum>
std::ostream& operator<<(std::ostream& os, const enum_type<Enum>& e)
{
    return os << e.value;
}

struct source_location_info
{
    source_location_info(std::source_location&& loc = std::source_location::current())
        : m_loc{ std::move(loc) }
    {}

    friend std::ostream& operator<<(std::ostream& os, const source_location_info& loc_info)
    {
        const auto& loc = loc_info.m_loc;
        return os << loc.function_name() << ':' << loc.line();
    }

private:
    const std::source_location m_loc;
};

struct handle_error
{
    constexpr handle_error(std::source_location&& loc = std::source_location::current()) noexcept
        : m_loc{ std::move(loc) }
    {}

    ~handle_error()
    {
        std::cout << m_loc.file_name() << ':' << m_loc.function_name() << ':' << m_loc.line();
        if (!m_msgBuffer.empty()) {
            std::cout << ": " << m_msgBuffer;
        }
        std::cout << '\n';
    }

    template<typename Error>
    [[nodiscard]] Error error(Error error)
    {
        return error;
    }

    [[nodiscard]] operator bool()
    {
        return false;
    }

    template<typename T>
    [[nodiscard]] operator std::optional<T>()
    {
        return std::nullopt;
    }

    template<typename T>
    [[nodiscard]] operator std::unique_ptr<T>()
    {
        return nullptr;
    }

    template<typename T>
    [[nodiscard]] operator std::shared_ptr<T>()
    {
        return nullptr;
    }

    [[nodiscard]] handle_error& operator<<(const auto& msg)
    {
        std::stringstream strm{};
        strm << msg;
        m_msgBuffer += strm.str();
        return *this;
    }

private:
    const std::source_location m_loc;
    std::string m_msgBuffer;
};

template<typename T>
concept callable = requires(T t) { t(); };

template<callable T>
struct scope_guard
{
    constexpr scope_guard(T&& callable) noexcept
        : m_callable{std::move(callable)}
    {}

    constexpr ~scope_guard()
    {
        try {
            m_callable();
        }
        catch (...) {
            // noop
        }
    }

private:
    T m_callable;
};

template<callable T>
scope_guard<T> make_scope_guard(T&& callable)
{
    return { std::move(callable) };
}

template<typename OutT, typename InT, template<typename...> class Container, typename Func, typename...Args>
Container<OutT> transform_each(const Container<InT, Args...>& in, Func&& func)
{
    using InContainer = Container<InT, Args...>;
    using OutContainer = Container<OutT>;

    OutContainer out{};
    out.reserve(in.size());
    std::ranges::transform(in, std::back_inserter(out), func);
    return out;
}

template<typename Char>
const Char* string_cstr(const std::basic_string<Char>& str)
{
    return str.c_str();
}

_MAKE_STREAM(fstream)
_MAKE_STREAM(ifstream)
_MAKE_STREAM(ostream)
_MAKE_STREAM(stringstream)
#undef _MAKE_STREAM

template<typename Char>
std::basic_string<Char> read_file_contents(const Char* path)
{
    auto ifstream = make_ifstream<Char>(path, std::ios::in);
    if (ifstream.is_open()) {
        const auto ifstream_guard = util::make_scope_guard([&ifstream]() { ifstream.close(); });
        auto sstr = make_stringstream<Char>();
        sstr << ifstream.rdbuf();
        return sstr.str();
    }
    return {};
}

template<typename Char>
std::basic_string<Char> read_file_contents(const std::basic_string_view<Char>& path)
{
    return read_file_contents(path.data());
}

template<typename To, typename From>
constexpr To static_cast_fn(const From& from)
{
    return static_cast<To>(from);
}

template<typename Container>
size_t contained_data_size(const Container& cont)
{
    return cont.size() * sizeof(Container::value_type);
}

template<typename String, String::value_type delim>
class sliding_window_iterator_adaptor
{
    using string_iterator = typename String::iterator;

private:
    class iterator
    {
    public:
        iterator() = default;

        iterator(const iterator& other)
            : pstr_{ other.pstr_ }
            , first{ other.first }
            , second{ other.second }
        {}

        iterator(String& str, string_iterator b, string_iterator e)
            : pstr_{ &str }
            , first{ b }
            , second{ e }
        {}

        iterator& operator=(const iterator& rhs)
        {
            pstr_ = rhs.pstr_;
            first = rhs.first;
            second = rhs.second;
            return *this;
        }

        iterator& operator++()
        {
            if (second != pstr_->end()) {
                first = second + 1;
                second = std::find(second + 1, pstr_->end(), delim);
                return *this;
            }
            assert(pstr_);
            return *this = iterator{ *pstr_, pstr_->end(), pstr_->end() };
        }

        std::basic_string_view<typename String::value_type> operator*()
        {
            return { first, second };
        }

        bool operator==(iterator rhs)
        {
            return first == rhs.first && second == rhs.second;
        }

    public:
        string_iterator first;
        string_iterator second;

    private:
        String* pstr_ = nullptr;
    };

public:
    sliding_window_iterator_adaptor(String& str)
    {
        const auto str_begin = str.begin();
        auto first_of = str.find_first_of(delim);
        const auto size = str.size();
        const auto npos = String::npos;
        first_of = (first_of != String::npos || first_of < str.size() - 1) ? first_of : str.size();
        begin_ = iterator{ str, str_begin, str_begin + first_of };

        const auto str_end = str.end();
        end_ = iterator{ str, str_end, str_end };
    }

    iterator begin()
    {
        return begin_;
    }

    iterator end()
    {
        return end_;
    }

private:
    iterator begin_;
    iterator end_;
};

template<typename String>
using line_iterator_adaptor = sliding_window_iterator_adaptor<String, '\n'>;

template<typename String>
using comma_iterator_adaptor = sliding_window_iterator_adaptor<String, ','>;

struct any_throw_t
{
    template<typename T>
    constexpr operator T()
    {
        throw std::runtime_error(nullptr);
        std::unreachable();
    }
};

inline any_throw_t any_throw;

template<typename To>
constexpr To string_to(std::string_view v)
{
    if constexpr (std::is_integral_v<To> && std::is_signed_v<To>) {
        return static_cast<To>(std::stoll(std::string{ v.begin(), v.end() }));
    }
    if constexpr (std::is_integral_v<To> && std::is_unsigned_v<To>) {
        return static_cast<To>(std::stoull(std::string{ v.begin(), v.end() }));
    }
    if constexpr (std::is_floating_point_v<To>) {
        return static_cast<To>(std::stold(std::string{ v.begin(), v.end() }));
    }
    if constexpr (std::is_same_v<To, std::string>) {
        return std::string{v.begin(), v.end()};
    }
    std::unreachable(); // todo
}

template<>
constexpr std::string_view string_to<std::string_view>(std::string_view v)
{
    return v;
}

template<typename Offset, typename Struct, typename Field>
constexpr Offset field_offset(Field Struct::* field)
{
    static Struct s{};
    return static_cast<Offset>(reinterpret_cast<uintptr_t>(&(s.*field)) - reinterpret_cast<uintptr_t>(&s));
}

} // namespace util

#endif // UTIL_HPP
