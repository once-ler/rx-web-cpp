/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_DETAIL_ANY_CONTEXT_HPP_INCLUDED
#define BUSTACHE_DETAIL_ANY_CONTEXT_HPP_INCLUDED

#include <string>
#include <utility>

namespace bustache
{
    struct format;
}

namespace bustache { namespace detail
{
    struct any_context
    {
        using value_type = std::pair<std::string const, format>;
        using iterator = value_type const*;

        template<class Context>
        any_context(Context const& context) noexcept
            : _data(&context), _find(find_fn<Context>)
        {}

        iterator find(std::string const& key) const
        {
            return _find(_data, key);
        }

        iterator end() const
        {
            return nullptr;
        }

    private:

        template<class Context>
        static value_type const* find_fn(void const* data, std::string const& key)
        {
            auto ctx = static_cast<Context const*>(data);
            auto it = ctx->find(key);
            return it != ctx->end() ? &*it : nullptr;
        }

        void const* _data;
        value_type const* (*_find)(void const*, std::string const&);
    };
}}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_DETAIL_VARIANT_HPP_INCLUDED
#define BUSTACHE_DETAIL_VARIANT_HPP_INCLUDED

#include <cassert>
#include <cstdlib>
#include <utility>
#include <stdexcept>
#include <type_traits>

namespace bustache { namespace detail
{
    template<class T>
    inline T& cast(void* data)
    {
        return *static_cast<T*>(data);
    }

    template<class T>
    inline T const& cast(void const* data)
    {
        return *static_cast<T const*>(data);
    }

    template<class T, class U>
    struct noexcept_ctor_assign
    {
        static constexpr bool value =
            std::is_nothrow_constructible<T, U>::value &&
            std::is_nothrow_assignable<T, U>::value;
    };

    struct ctor_visitor
    {
        using result_type = void;

        void* data;

        template<class T>
        void operator()(T& t) const
        {
            new(data) T(std::move(t));
        }

        template<class T>
        void operator()(T const& t) const
        {
            new(data) T(t);
        }
    };

    struct assign_visitor
    {
        using result_type = void;

        void* data;

        template<class T>
        void operator()(T& t) const
        {
            *static_cast<T*>(data) = std::move(t);
        }

        template<class T>
        void operator()(T const& t) const
        {
            *static_cast<T*>(data) = t;
        }
    };

    struct dtor_visitor
    {
        using result_type = void;

        template<class T>
        void operator()(T& t) const
        {
            t.~T();
        }
    };

    template<class T>
    struct type {};
}}

namespace bustache
{
    template<class T>
    struct variant_base {};

    template<class View>
    struct variant_ptr
    {
        variant_ptr() noexcept : _data() {}

        variant_ptr(std::nullptr_t) noexcept : _data() {}

        variant_ptr(unsigned which, void const* data) noexcept
            : _which(which), _data(data)
        {}

        explicit operator bool() const
        {
            return !!_data;
        }

        View operator*() const
        {
            return{_which, _data};
        }

        unsigned which() const
        {
            return _which;
        }

        void const* data() const
        {
            return _data;
        }

    private:

        unsigned _which;
        void const* _data;
    };

    class bad_variant_access : public std::exception
    {
    public:
        bad_variant_access() noexcept {}

        const char* what() const noexcept override
        {
            return "bustache::bad_variant_access";
        }
    };

    template<class Visitor, class Var>
    inline decltype(auto) visit(Visitor&& visitor, variant_base<Var>& v)
    {
        auto& var = static_cast<Var&>(v);
        return Var::switcher::visit(var.which(), var.data(), visitor);
    }

    template<class Visitor, class Var>
    inline decltype(auto) visit(Visitor&& visitor, variant_base<Var> const& v)
    {
        auto& var = static_cast<Var const&>(v);
        return Var::switcher::visit(var.which(), var.data(), visitor);
    }

    // Synomym of visit (for Boost.Variant compatibility)
    template<class Visitor, class Var>
    inline decltype(auto) apply_visitor(Visitor&& visitor, variant_base<Var>& v)
    {
        return visit(std::forward<Visitor>(visitor), v);
    }

    template<class Visitor, class Var>
    inline decltype(auto) apply_visitor(Visitor&& visitor, variant_base<Var> const& v)
    {
        return visit(std::forward<Visitor>(visitor), v);
    }

    template<class T, class Var>
    inline T& get(variant_base<Var>& v)
    {
        auto& var = static_cast<Var&>(v);
        if (Var::switcher::index(detail::type<T>{}) == var.which())
            return *static_cast<T*>(var.data());
        throw bad_variant_access();
    }

    template<class T, class Var>
    inline T const& get(variant_base<Var> const& v)
    {
        auto& var = static_cast<Var const&>(v);
        if (Var::switcher::index(detail::type<T>{}) == var.which())
            return *static_cast<T const*>(var.data());
        throw bad_variant_access();
    }

    template<class T, class Var>
    inline T* get(variant_base<Var>* vp)
    {
        if (vp)
        {
            auto v = static_cast<Var*>(vp);
            if (Var::switcher::index(detail::type<T>{}) == v->which())
                return static_cast<T*>(v->data());
        }
        return nullptr;
    }

    template<class T, class Var>
    inline T const* get(variant_base<Var> const* vp)
    {
        if (vp)
        {
            auto v = static_cast<Var const*>(vp);
            if (Var::switcher::index(detail::type<T>{}) == v->which())
                return static_cast<T const*>(v->data());
        }
        return nullptr;
    }

    template<class T, class Var>
    inline T const* get(variant_ptr<Var> const& vp)
    {
        if (vp)
        {
            if (Var::switcher::index(detail::type<T>{}) == vp.which())
                return static_cast<T const*>(vp.data());
        }
        return nullptr;
    }
}


#define Zz_BUSTACHE_UNREACHABLE(MSG) { assert(!MSG); std::abort(); }
#define Zz_BUSTACHE_VARIANT_SWITCH(N, U, D) case N: return v(detail::cast<U>(data));
#if 0 // Common type deduction, not used for now
#define Zz_BUSTACHE_VARIANT_RET(N, U, D) true ? v(detail::cast<U>(data)) :
    // Put this into switcher before visit
    template<class T, class Visitor>                                            \
    static auto common_ret(T* data, Visitor& v) ->                              \
        decltype(TYPES(Zz_BUSTACHE_VARIANT_RET,) throw bad_variant_access());   \
    /***/
#endif
#define Zz_BUSTACHE_VARIANT_MEMBER(N, U, D) U _##N;
#define Zz_BUSTACHE_VARIANT_CTOR(N, U, D)                                       \
D(U val) noexcept : _which(N), _##N(std::move(val)) {}
/***/
#define Zz_BUSTACHE_VARIANT_INDEX(N, U, D)                                      \
static constexpr unsigned index(detail::type<U>) { return N; }                  \
/***/
#define Zz_BUSTACHE_VARIANT_MATCH(N, U, D) static U match_type(U);
#define Zz_BUSTACHE_VARIANT_DECL(VAR, TYPES, NOEXCPET)                          \
struct switcher                                                                 \
{                                                                               \
    template<class T, class Visitor>                                            \
    static decltype(auto) visit(unsigned which, T* data, Visitor& v)            \
    {                                                                           \
        switch (which)                                                          \
        {                                                                       \
        TYPES(Zz_BUSTACHE_VARIANT_SWITCH,)                                      \
        default: throw bad_variant_access();                                    \
        }                                                                       \
    }                                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_INDEX,)                                           \
};                                                                              \
private:                                                                        \
TYPES(Zz_BUSTACHE_VARIANT_MATCH,)                                               \
unsigned _which;                                                                \
union                                                                           \
{                                                                               \
    char _storage[1];                                                           \
    TYPES(Zz_BUSTACHE_VARIANT_MEMBER,)                                          \
};                                                                              \
void invalidate()                                                               \
{                                                                               \
    if (valid())                                                                \
    {                                                                           \
        detail::dtor_visitor v;                                                 \
        switcher::visit(_which, data(), v);                                     \
        _which = ~0u;                                                           \
    }                                                                           \
}                                                                               \
template<class T>                                                               \
void do_init(T& other)                                                          \
{                                                                               \
    detail::ctor_visitor v{_storage};                                           \
    switcher::visit(other._which, other.data(), v);                             \
}                                                                               \
template<class T>                                                               \
void do_assign(T& other)                                                        \
{                                                                               \
    if (_which == other._which)                                                 \
    {                                                                           \
        detail::assign_visitor v{_storage};                                     \
        switcher::visit(other._which, other.data(), v);                         \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        invalidate();                                                           \
        if (other.valid())                                                      \
        {                                                                       \
            do_init(other);                                                     \
            _which = other._which;                                              \
        }                                                                       \
    }                                                                           \
}                                                                               \
public:                                                                         \
unsigned which() const                                                          \
{                                                                               \
    return _which;                                                              \
}                                                                               \
bool valid() const                                                              \
{                                                                               \
    return _which != ~0u;                                                       \
}                                                                               \
void* data()                                                                    \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
void const* data() const                                                        \
{                                                                               \
    return _storage;                                                            \
}                                                                               \
VAR(VAR&& other) noexcept(NOEXCPET) : _which(other._which)                      \
{                                                                               \
    do_init(other);                                                             \
}                                                                               \
VAR(VAR const& other) : _which(other._which)                                    \
{                                                                               \
    do_init(other);                                                             \
}                                                                               \
template<class T, class U = decltype(match_type(std::declval<T>()))>            \
VAR(T&& other) noexcept(std::is_nothrow_constructible<U, T>::value)             \
  : _which(switcher::index(detail::type<U>{}))                                  \
{                                                                               \
    new(_storage) U(std::forward<T>(other));                                    \
}                                                                               \
~VAR()                                                                          \
{                                                                               \
    if (valid())                                                                \
    {                                                                           \
        detail::dtor_visitor v;                                                 \
        switcher::visit(_which, data(), v);                                     \
    }                                                                           \
}                                                                               \
template<class T, class U = decltype(match_type(std::declval<T>()))>            \
U& operator=(T&& other) noexcept(detail::noexcept_ctor_assign<U, T>::value)     \
{                                                                               \
    if (switcher::index(detail::type<U>{}) == _which)                           \
        return *static_cast<U*>(data()) = std::forward<T>(other);               \
    else                                                                        \
    {                                                                           \
        invalidate();                                                           \
        auto p = new(_storage) U(std::forward<T>(other));                       \
        _which = switcher::index(detail::type<U>{});                            \
        return *p;                                                              \
    }                                                                           \
}                                                                               \
VAR& operator=(VAR&& other) noexcept(NOEXCPET)                                  \
{                                                                               \
    do_assign(other);                                                           \
    return *this;                                                               \
}                                                                               \
VAR& operator=(VAR const& other)                                                \
{                                                                               \
    do_assign(other);                                                           \
    return *this;                                                               \
}                                                                               \
/***/

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_AST_HPP_INCLUDED
#define BUSTACHE_AST_HPP_INCLUDED


#include <boost/utility/string_ref.hpp>
#include <boost/unordered_map.hpp>
#include <vector>
#include <string>

namespace bustache { namespace ast
{
    struct variable;
    struct section;
    struct content;

    using text = boost::string_ref;

    using content_list = std::vector<content>;

    using override_map = boost::unordered_map<std::string, content_list>;

    struct null {};

    struct variable
    {
        std::string key;
        char tag = '\0';
#ifdef _MSC_VER // Workaround MSVC bug.
        variable() = default;

        explicit variable(std::string key, char tag = '\0')
          : key(std::move(key)), tag(tag)
        {}
#endif
    };

    struct block
    {
        std::string key;
        content_list contents;
    };

    struct section : block
    {
        char tag = '#';
    };

    struct partial
    {
        std::string key;
        std::string indent;
        override_map overriders;
    };

#define BUSTACHE_AST_CONTENT(X, D)                                              \
    X(0, null, D)                                                               \
    X(1, text, D)                                                               \
    X(2, variable, D)                                                           \
    X(3, section, D)                                                            \
    X(4, partial, D)                                                            \
    X(5, block, D)                                                              \
/***/

    struct content : variant_base<content>
    {
        Zz_BUSTACHE_VARIANT_DECL(content, BUSTACHE_AST_CONTENT, true)

        content() noexcept : _which(0), _0() {}
    };
#undef BUSTACHE_AST_CONTENT

    inline bool is_null(content const& c)
    {
        return !c.which();
    }
}}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_FORMAT_HPP_INCLUDED
#define BUSTACHE_FORMAT_HPP_INCLUDED


#include <stdexcept>
#include <memory>

namespace bustache
{
    struct format;
    
    using option_type = bool;
    constexpr option_type normal = false;
    constexpr option_type escape_html = true;

    template<class T, class Context>
    struct manipulator
    {
        format const& fmt;
        T const& data;
        Context const& context;
        option_type const flag;
    };

    struct no_context
    {
        using value_type = std::pair<std::string const, format>;
        using iterator = value_type const*;
        
        constexpr iterator find(std::string const&) const
        {
            return nullptr;
        }
        
        constexpr iterator end() const
        {
            return nullptr;
        }

        static no_context const& dummy()
        {
            static no_context const _{};
            return _;
        }
    };

    enum error_type
    {
        error_set_delim,
        error_baddelim,
        error_delim,
        error_section,
        error_badkey
    };

    class format_error : public std::runtime_error
    {
        error_type _err;

    public:
        explicit format_error(error_type err);

        error_type code() const
        {
            return _err;
        }
    };
    
    struct format
    {
        format() = default;

        format(char const* begin, char const* end)
        {
            init(begin, end);
        }
        
        template<class Source>
        explicit format(Source const& source)
        {
            init(source.data(), source.data() + source.size());
        }
        
        template<class Source>
        explicit format(Source const&& source)
        {
            init(source.data(), source.data() + source.size());
            copy_text(text_size());
        }

        template<std::size_t N>
        explicit format(char const (&source)[N])
        {
            init(source, source + (N - 1));
        }

        explicit format(ast::content_list contents, bool copytext = true)
          : _contents(std::move(contents))
        {
            if (copytext)
                copy_text(text_size());
        }

        format(format&& other) noexcept
          : _contents(std::move(other._contents)), _text(std::move(other._text))
        {}

        format(format const& other) : _contents(other._contents)
        {
            if (other._text)
                copy_text(text_size());
        }

        template<class T>
        manipulator<T, no_context>
        operator()(T const& data, option_type flag = normal) const
        {
            return {*this, data, no_context::dummy(), flag};
        }
        
        template<class T, class Context>
        manipulator<T, Context>
        operator()(T const& data, Context const& context, option_type flag = normal) const
        {
            return {*this, data, context, flag};
        }
        
        ast::content_list const& contents() const
        {
            return _contents;
        }
        
    private:
        
        void init(char const* begin, char const* end);
        std::size_t text_size() const;
        void copy_text(std::size_t n);

        ast::content_list _contents;
        std::unique_ptr<char[]> _text;
    };

    inline namespace literals
    {
        inline format operator"" _fmt(char const* str, std::size_t n)
        {
            return format(str, str + n);
        }
    }
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_DEBUG_HPP_INCLUDED
#define BUSTACHE_DEBUG_HPP_INCLUDED

#include <iostream>
#include <iomanip>


namespace bustache { namespace detail
{
    template<class CharT, class Traits>
    struct ast_printer
    {
        std::basic_ostream<CharT, Traits>& out;
        unsigned level;
        unsigned const space;

        void operator()(ast::text const& text) const
        {
            indent();
            auto i = text.begin();
            auto i0 = i;
            auto e = text.end();
            out << "text: \"";
            while (i != e)
            {
                char const* esc = nullptr;
                switch (*i)
                {
                case '\r': esc = "\\r"; break;
                case '\n': esc = "\\n"; break;
                case '\\': esc = "\\\\"; break;
                default: ++i; continue;
                }
                out.write(i0, i - i0);
                i0 = ++i;
                out << esc;
            }
            out.write(i0, i - i0);
            out << "\"\n";
        }

        void operator()(ast::variable const& variable) const
        {
            indent();
            out << "variable";
            if (variable.tag)
                out << "(&)";
            out << ": " << variable.key << "\n";
        }

        void operator()(ast::section const& section)
        {
            out;
            out << "section(" << section.tag << "): " << section.key << "\n";
            ++level;
            for (auto const& content : section.contents)
                apply_visitor(*this, content);
            --level;
        }

        void operator()(ast::partial const& partial) const
        {
            out << "partial: " << partial.key << "\n";
        }

        void operator()(ast::null) const {} // never called

        void indent() const
        {
            out << std::setw(space * level) << "";
        }
    };
}}

namespace bustache
{
    template<class CharT, class Traits>
    inline void print_ast(std::basic_ostream<CharT, Traits>& out, format const& fmt, unsigned indent = 4)
    {
        detail::ast_printer<CharT, Traits> visitor{out, 0, indent};
        for (auto const& content : fmt.contents())
            apply_visitor(visitor, content);
    }
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_MODEL_HPP_INCLUDED
#define BUSTACHE_MODEL_HPP_INCLUDED




#include <vector>
#include <functional>
#include <boost/unordered_map.hpp>

namespace bustache
{
    class value;

    using array = std::vector<value>;

    // We use boost::unordered_map because it allows incomplete type.
    using object = boost::unordered_map<std::string, value>;

    using lambda0v = std::function<value()>;

    using lambda0f = std::function<format()>;

    using lambda1v = std::function<value(ast::content_list const&)>;

    using lambda1f = std::function<format(ast::content_list const&)>;

#define BUSTACHE_VALUE(X, D)                                                    \
    X(0, std::nullptr_t, D)                                                     \
    X(1, bool, D)                                                               \
    X(2, int, D)                                                                \
    X(3, double, D)                                                             \
    X(4, std::string, D)                                                        \
    X(5, array, D)                                                              \
    X(6, lambda0v, D)                                                           \
    X(7, lambda0f, D)                                                           \
    X(8, lambda1v, D)                                                           \
    X(9, lambda1f, D)                                                           \
    X(10, object, D)                                                            \
/***/

    class value : public variant_base<value>
    {
        // Need to override for `char const*`, otherwise `bool` will be chosen
        static std::string match_type(char const*);

    public:

        struct view;
        using pointer = variant_ptr<view>;

        Zz_BUSTACHE_VARIANT_DECL(value, BUSTACHE_VALUE, false)

        value() noexcept : _which(0), _0() {}

        pointer get_pointer() const
        {
            return {_which, _storage};
        }
    };

    struct value::view : variant_base<view>
    {
        using switcher = value::switcher;

#define BUSTACHE_VALUE_VIEW_CTOR(N, U, D)                                       \
        view(U const& data) noexcept : _which(N), _data(&data) {}
        BUSTACHE_VALUE(BUSTACHE_VALUE_VIEW_CTOR,)
#undef BUSTACHE_VALUE_VIEW_CTOR

        view(value const& data) noexcept
          : _which(data._which), _data(data._storage)
        {}

        view(unsigned which, void const* data) noexcept
          : _which(which), _data(data)
        {}

        unsigned which() const
        {
            return _which;
        }

        void const* data() const
        {
            return _data;
        }

        pointer get_pointer() const
        {
            return {_which, _data};
        }

    private:

        unsigned _which;
        void const* _data;
    };
#undef BUSTACHE_VALUE
}

namespace bustache
{
    // Forward decl only.
    template<class CharT, class Traits, class Context>
    void generate_ostream
    (
        std::basic_ostream<CharT, Traits>& out, format const& fmt,
        value::view const& data, Context const& context, option_type flag
    );

    // Forward decl only.
    template<class String, class Context>
    void generate_string
    (
        String& out, format const& fmt,
        value::view const& data, Context const& context, option_type flag
    );

    template<class CharT, class Traits, class T, class Context,
        std::enable_if_t<std::is_constructible<value::view, T>::value, bool> = true>
    inline std::basic_ostream<CharT, Traits>&
    operator<<(std::basic_ostream<CharT, Traits>& out, manipulator<T, Context> const& manip)
    {
        generate_ostream(out, manip.fmt, manip.data, detail::any_context(manip.context), manip.flag);
        return out;
    }

    template<class T, class Context,
        std::enable_if_t<std::is_constructible<value::view, T>::value, bool> = true>
    inline std::string to_string(manipulator<T, Context> const& manip)
    {
        std::string ret;
        generate_string(ret, manip.fmt, manip.data, detail::any_context(manip.context), manip.flag);
        return ret;
    }
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_HPP_INCLUDED
#define BUSTACHE_GENERATE_HPP_INCLUDED



namespace bustache { namespace detail
{
    inline value::pointer find(object const& data, std::string const& key)
    {
        auto it = data.find(key);
        if (it != data.end())
            return it->second.get_pointer();
        return nullptr;
    }

    template<class Sink>
    struct value_printer
    {
        typedef void result_type;
        
        Sink const& sink;
        bool const escaping;

        void operator()(std::nullptr_t) const {}
        
        template<class T>
        void operator()(T data) const
        {
            sink(data);
        }

        void operator()(std::string const& data) const
        {
            auto it = data.data(), end = it + data.size();
            if (escaping)
                escape_html(it, end);
            else
                sink(it, end);
        }
        
        void operator()(array const& data) const
        {
            auto it = data.begin(), end = data.end();
            if (it != end)
            {
                visit(*this, *it);
                while (++it != end)
                {
                    literal(",");
                    visit(*this, *it);
                }
            }
        }

        void operator()(object const&) const
        {
            literal("[Object]");
        }

        void operator()(lambda0v const& data) const
        {
            visit(*this, data());
        }

        void operator()(lambda1v const& data) const
        {
            visit(*this, data({}));
        }

        template<class Sig>
        void operator()(std::function<Sig> const&) const
        {
            literal("[Function]");
        }

        void escape_html(char const* it, char const* end) const
        {
            char const* last = it;
            while (it != end)
            {
                switch (*it)
                {
                case '&': sink(last, it); literal("&amp;"); break;
                case '<': sink(last, it); literal("&lt;"); break;
                case '>': sink(last, it); literal("&gt;"); break;
                case '\\': sink(last, it); literal("&#92;"); break;
                case '"': sink(last, it); literal("&quot;"); break;
                default:  ++it; continue;
                }
                last = ++it;
            }
            sink(last, it);
        }

        template<std::size_t N>
        void literal(char const (&str)[N]) const
        {
            sink(str, str + (N - 1));
        }
    };

    struct content_scope
    {
        content_scope const* const parent;
        object const& data;

        value::pointer lookup(std::string const& key) const
        {
            if (auto pv = find(data, key))
                return pv;
            if (parent)
                return parent->lookup(key);
            return nullptr;
        }
    };

    struct content_visitor_base
    {
        using result_type = void;

        content_scope const* scope;
        value::pointer cursor;
        std::vector<ast::override_map const*> chain;
        mutable std::string key_cache;

        // Defined in src/generate.cpp.
        value::pointer resolve(std::string const& key) const;

        ast::content_list const* find_override(std::string const& key) const
        {
            for (auto pm : chain)
            {
                auto it = pm->find(key);
                if (it != pm->end())
                    return &it->second;
            }
            return nullptr;
        }
    };

    template<class ContentVisitor>
    struct variable_visitor : value_printer<typename ContentVisitor::sink_type>
    {
        using base_type = value_printer<typename ContentVisitor::sink_type>;
        
        ContentVisitor& parent;

        variable_visitor(ContentVisitor& parent, bool escaping)
          : base_type{parent.sink, escaping}, parent(parent)
        {}

        using base_type::operator();

        void operator()(lambda0f const& data) const
        {
            auto fmt(data());
            for (auto const& content : fmt.contents())
                visit(parent, content);
        }
    };

    template<class ContentVisitor>
    struct section_visitor
    {
        using result_type = bool;

        ContentVisitor& parent;
        ast::content_list const& contents;
        bool const inverted;

        bool operator()(object const& data) const
        {
            if (!inverted)
            {
                content_scope scope{parent.scope, data};
                auto old_scope = parent.scope;
                parent.scope = &scope;
                for (auto const& content : contents)
                    visit(parent, content);
                parent.scope = old_scope;
            }
            return false;
        }

        bool operator()(array const& data) const
        {
            if (inverted)
                return data.empty();

            for (auto const& val : data)
            {
                parent.cursor = val.get_pointer();
                if (auto obj = get<object>(&val))
                {
                    content_scope scope{parent.scope, *obj};
                    auto old_scope = parent.scope;
                    parent.scope = &scope;
                    for (auto const& content : contents)
                        visit(parent, content);
                    parent.scope = old_scope;
                }
                else
                {
                    for (auto const& content : contents)
                        visit(parent, content);
                }
            }
            return false;
        }

        bool operator()(bool data) const
        {
            return data ^ inverted;
        }

        // The 2 overloads below are not necessary but to suppress
        // the stupid MSVC warning.
        bool operator()(int data) const
        {
            return !!data ^ inverted;
        }

        bool operator()(double data) const
        {
            return !!data ^ inverted;
        }

        bool operator()(std::string const& data) const
        {
            return !data.empty() ^ inverted;
        }

        bool operator()(std::nullptr_t) const
        {
            return inverted;
        }

        bool operator()(lambda0v const& data) const
        {
            return inverted ? false : visit(*this, data());
        }

        bool operator()(lambda0f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data());
                for (auto const& content : fmt.contents())
                    visit(parent, content);
            }
            return false;
        }

        bool operator()(lambda1v const& data) const
        {
            return inverted ? false : visit(*this, data(contents));
        }

        bool operator()(lambda1f const& data) const
        {
            if (!inverted)
            {
                auto fmt(data(contents));
                for (auto const& content : fmt.contents())
                    visit(parent, content);
            }
            return false;
        }
    };

    template<class Sink, class Context>
    struct content_visitor : content_visitor_base
    {
        using sink_type = Sink;

        Sink const& sink;
        Context const& context;
        std::string indent;
        bool needs_indent;
        bool const escaping;

        content_visitor
        (
            content_scope const& scope, value::pointer cursor,
            Sink const &sink, Context const &context, bool escaping
        )
          : content_visitor_base{&scope, cursor, {}, {}}
          , sink(sink), context(context), needs_indent(), escaping(escaping)
        {}

        void operator()(ast::text const& text)
        {
            auto i = text.begin();
            auto e = text.end();
            assert(i != e && "empty text shouldn't be in ast");
            if (indent.empty())
            {
                sink(i, e);
                return;
            }
            --e; // Don't flush indent on last newline.
            auto const ib = indent.data();
            auto const ie = ib + indent.size();
            if (needs_indent)
                sink(ib, ie);
            auto i0 = i;
            while (i != e)
            {
                if (*i++ == '\n')
                {
                    sink(i0, i);
                    sink(ib, ie);
                    i0 = i;
                }
            }
            needs_indent = *i++ == '\n';
            sink(i0, i);
        }
        
        void operator()(ast::variable const& variable)
        {
            if (auto pv = resolve(variable.key))
            {
                if (needs_indent)
                {
                    sink(indent.data(), indent.data() + indent.size());
                    needs_indent = false;
                }
                variable_visitor<content_visitor> visitor
                {
                    *this, escaping && !variable.tag
                };
                visit(visitor, *pv);
            }
        }
        
        void operator()(ast::section const& section)
        {
            bool inverted = section.tag == '^';
            auto old_cursor = cursor;
            if (auto next = resolve(section.key))
            {
                cursor = next;
                section_visitor<content_visitor> visitor
                {
                    *this, section.contents, inverted
                };
                if (!visit(visitor, *cursor))
                {
                    cursor = old_cursor;
                    return;
                }
            }
            else if (!inverted)
                return;
                
            for (auto const& content : section.contents)
                visit(*this, content);
            cursor = old_cursor;
        }
        
        void operator()(ast::partial const& partial)
        {
            auto it = context.find(partial.key);
            if (it != context.end())
            {
                if (it->second.contents().empty())
                    return;

                auto old_size = indent.size();
                auto old_chain = chain.size();
                indent += partial.indent;
                needs_indent |= !partial.indent.empty();
                if (!partial.overriders.empty())
                    chain.push_back(&partial.overriders);
                for (auto const& content : it->second.contents())
                    visit(*this, content);
                chain.resize(old_chain);
                indent.resize(old_size);
            }
        }

        void operator()(ast::block const& block)
        {
            auto pc = find_override(block.key);
            if (!pc)
                pc = &block.contents;
            for (auto const& content : *pc)
                visit(*this, content);
        }

        void operator()(ast::null) const {} // never called
    };
}}

namespace bustache
{
    template<class Sink>
    inline void generate
    (
        Sink& sink, format const& fmt, value::view const& data,
        option_type flag = normal
    )
    {
        generate(sink, fmt, data, no_context::dummy(), flag);
    }
    
    template<class Sink, class Context>
    void generate
    (
        Sink& sink, format const& fmt, value::view const& data,
        Context const& context, option_type flag = normal
    )
    {
        object const empty;
        auto obj = get<object>(&data);
        detail::content_scope scope{nullptr, obj ? *obj : empty};
        detail::content_visitor<Sink, Context> visitor{scope, data.get_pointer(), sink, context, flag};
        for (auto const& content : fmt.contents())
            visit(visitor, content);
    }
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_OSTREAM_HPP_INCLUDED
#define BUSTACHE_GENERATE_OSTREAM_HPP_INCLUDED

#include <iostream>


namespace bustache { namespace detail
{
    template<class CharT, class Traits>
    struct ostream_sink
    {
        std::basic_ostream<CharT, Traits>& out;

        void operator()(char const* it, char const* end) const
        {
            out.write(it, end - it);
        }

        template<class T>
        void operator()(T data) const
        {
            out << data;
        }

        void operator()(bool data) const
        {
            out << (data ? "true" : "false");
        }
    };
}}

namespace bustache
{
    template<class CharT, class Traits, class Context>
    void generate_ostream
    (
        std::basic_ostream<CharT, Traits>& out, format const& fmt,
        value::view const& data, Context const& context, option_type flag
    )
    {
        detail::ostream_sink<CharT, Traits> sink{out};
        generate(sink, fmt, data, context, flag);
    }

    // This is instantiated in src/generate.cpp.
    extern template
    void generate_ostream
    (
        std::ostream& out, format const& fmt,
        value::view const& data, detail::any_context const& context, option_type flag
    );
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#ifndef BUSTACHE_GENERATE_STRING_HPP_INCLUDED
#define BUSTACHE_GENERATE_STRING_HPP_INCLUDED

#include <cstdio> // for snprintf
#include <string>


namespace bustache { namespace detail
{
    template<class String>
    struct string_sink
    {
        String& out;

        void operator()(char const* it, char const* end) const
        {
            out.insert(out.end(), it, end);
        }

        void operator()(int data) const
        {
            append_num("%d", data);
        }

        void operator()(double data) const
        {
            append_num("%g", data);
        }

        void operator()(bool data) const
        {
            data ? append("true") : append("false");
        }

        template<std::size_t N>
        void append(char const (&str)[N]) const
        {
            out.insert(out.end(), str, str + (N - 1));
        }

        template<class T>
        void append_num(char const* fmt, T data) const
        {
            char buf[64];
            char* p;
            auto old_size = out.size();
            auto capacity = out.capacity();
            auto bufsize = capacity - old_size;
            if (bufsize)
            {
                out.resize(capacity);
                p = &out.front() + old_size;
            }
            else
            {
                bufsize = sizeof(buf);
                p = buf;
            }
            auto n = std::snprintf(p, bufsize, fmt, data);
            if (n < 0) // error
                return;
            if (unsigned(n + 1) <= bufsize)
            {
                if (p == buf)
                {
                    out.insert(out.end(), p, p + n);
                    return;
                }
            }
            else
            {
                out.resize(old_size + n + 1); // '\0' will be written
                std::snprintf(&out.front() + old_size, n + 1, fmt, data);
            }
            out.resize(old_size + n);
        }
    };
}}

namespace bustache
{
    template<class String, class Context>
    void generate_string
    (
        String& out, format const& fmt,
        value::view const& data, Context const& context, option_type flag
    )
    {
        detail::string_sink<String> sink{out};
        generate(sink, fmt, data, context, flag);
    }

    // This is instantiated in src/generate.cpp.
    extern template
    void generate_string
    (
        std::string& out, format const& fmt,
        value::view const& data, detail::any_context const& context, option_type flag
    );
}

#endif/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2014-2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/
#include <cctype>
#include <utility>
#include <cstring>


namespace bustache { namespace parser { namespace
{
    using delim = std::pair<std::string, std::string>;

    template<class I>
    inline void skip(I& i, I e)
    {
        while (i != e && std::isspace(*i))
            ++i;
    }

    template<class I>
    inline bool parse_char(I& i, I e, char c)
    {
        if (i != e && *i == c)
        {
            ++i;
            return true;
        }
        return false;
    }

    template<class I>
    inline bool parse_lit(I& i, I e, boost::string_ref const& str)
    {
        I i0 = i;
        for (char c : str)
        {
            if (!parse_char(i, e, c))
            {
                i = i0;
                return false;
            }
        }
        return true;
    }

    template<class I>
    void expect_key(I& i, I e, delim& d, std::string& attr, bool suffix)
    {
        skip(i, e);
        I i0 = i;
        while (i != e)
        {
            I i1 = i;
            skip(i, e);
            if (!suffix || parse_char(i, e, '}'))
            {
                skip(i, e);
                if (parse_lit(i, e, d.second))
                {
                    attr.assign(i0, i1);
                    if (i0 == i1)
                        throw format_error(error_badkey);
                    return;
                }
            }
            if (i != e)
                ++i;
        }
        throw format_error(error_badkey);
    }

    template<class I>
    bool parse_content
    (
        I& i0, I& i, I e, delim& d, bool& pure,
        boost::string_ref& text, ast::content& attr,
        boost::string_ref const& section
    );

    template<class I>
    void parse_contents
    (
        I i0, I& i, I e, delim& d, bool& pure,
        ast::content_list& attr, boost::string_ref const& section
    );

    template<class I>
    I process_pure(I& i, I e, bool& pure)
    {
        I i0 = i;
        if (pure)
        {
            while (i != e)
            {
                if (*i == '\n')
                {
                    i0 = ++i;
                    break;
                }
                else if (std::isspace(*i))
                    ++i;
                else
                {
                    pure = false;
                    break;
                }
            }
        }
        return i0;
    }

    template<class I>
    inline bool expect_block(I& i, I e, delim& d, bool& pure, ast::block& attr)
    {
        expect_key(i, e, d, attr.key, false);
        I i0 = process_pure(i, e, pure);
        bool standalone = pure;
        parse_contents(i0, i, e, d, pure, attr.contents, attr.key);
        return standalone;
    }

    template<class I>
    bool expect_inheritance(I& i, I e, delim& d, bool& pure, ast::partial& attr)
    {
        expect_key(i, e, d, attr.key, false);
        I i0 = process_pure(i, e, pure);
        bool standalone = pure;
        for (boost::string_ref text;;)
        {
            ast::content a;
            auto end = parse_content(i0, i, e, d, pure, text, a, attr.key);
            if (auto p = get<ast::block>(&a))
                attr.overriders.emplace(std::move(p->key), std::move(p->contents));
            if (end)
                break;
        }
        return standalone;
    }

    template<class I>
    void expect_comment(I& i, I e, delim& d)
    {
        while (!parse_lit(i, e, d.second))
        {
            if (i == e)
                throw format_error(error_delim);
            ++i;
        }
    }

    template<class I>
    void expect_set_delim(I& i, I e, delim& d)
    {
        skip(i, e);
        I i0 = i;
        while (i != e)
        {
            if (std::isspace(*i))
                break;
            ++i;
        }
        if (i == e)
            throw format_error(error_baddelim);
        d.first.assign(i0, i);
        skip(i, e);
        i0 = i;
        I i1 = i;
        for (;; ++i)
        {
            if (i == e)
                throw format_error(error_set_delim);
            if (*i == '=')
            {
                i1 = i;
                break;
            }
            if (std::isspace(*i))
            {
                i1 = i;
                skip(++i, e);
                if (i == e || *i != '=')
                    throw format_error(error_set_delim);
                break;
            }
        }
        if (i0 == i1)
            throw format_error(error_baddelim);
        std::string new_close(i0, i1);
        skip(++i, e);
        if (!parse_lit(i, e, d.second))
            throw format_error(error_delim);
        d.second = std::move(new_close);
    }

    struct tag_result
    {
        bool is_end_section;
        bool check_standalone;
        bool is_standalone;
    };

    template<class I>
    tag_result expect_tag
    (
        I& i, I e, delim& d, bool& pure,
        ast::content& attr, boost::string_ref const& section
    )
    {
        skip(i, e);
        if (i == e)
            throw format_error(error_badkey);
        tag_result ret{};
        switch (*i)
        {
        case '#':
        case '^':
        {
            ast::section a;
            a.tag = *i;
            ret.is_standalone = expect_block(++i, e, d, pure, a);
            attr = std::move(a);
            return ret;
        }
        case '/':
            skip(++i, e);
            if (section.empty() || !parse_lit(i, e, section))
                throw format_error(error_section);
            skip(i, e);
            if (!parse_lit(i, e, d.second))
                throw format_error(error_delim);
            ret.check_standalone = pure;
            ret.is_end_section = true;
            break;
        case '!':
        {
            expect_comment(++i, e, d);
            ret.check_standalone = pure;
            break;
        }
        case '=':
        {
            expect_set_delim(++i, e, d);
            ret.check_standalone = pure;
            break;
        }
        case '>':
        {
            ast::partial a;
            expect_key(++i, e, d, a.key, false);
            attr = std::move(a);
            ret.check_standalone = pure;
            break;
        }
        case '&':
        case '{':
        {
            ast::variable a;
            a.tag = *i;
            expect_key(++i, e, d, a.key, a.tag == '{');
            attr = std::move(a);
            pure = false;
            break;
        }
        // Extensions
        case '<':
        {
            ast::partial a;
            ret.is_standalone = expect_inheritance(++i, e, d, pure, a);
            attr = std::move(a);
            return ret;
        }
        case '$':
        {
            ast::block a;
            ret.is_standalone = expect_block(++i, e, d, pure, a);
            attr = std::move(a);
            return ret;
        }
        default:
            ast::variable a;
            expect_key(i, e, d, a.key, false);
            attr = std::move(a);
            pure = false;
            break;
        }
        return ret;
    }

    // return true if it ends
    template<class I>
    bool parse_content
    (
        I& i0, I& i, I e, delim& d, bool& pure,
        boost::string_ref& text, ast::content& attr,
        boost::string_ref const& section
    )
    {
        for (I i1 = i; i != e;)
        {
            if (*i == '\n')
            {
                pure = true;
                i1 = ++i;
            }
            else if (std::isspace(*i))
                ++i;
            else
            {
                I i2 = i;
                if (parse_lit(i, e, d.first))
                {
                    tag_result tag(expect_tag(i, e, d, pure, attr, section));
                    text = boost::string_ref(i0, i1 - i0);
                    if (tag.check_standalone)
                    {
                        I i3 = i;
                        while (i != e)
                        {
                            if (*i == '\n')
                            {
                                ++i;
                                break;
                            }
                            else if (std::isspace(*i))
                                ++i;
                            else
                            {
                                pure = false;
                                text = boost::string_ref(i0, i2 - i0);
                                // For end-section, we move the current pos (i)
                                // since i0 is local to the section and is not
                                // propagated upwards.
                                (tag.is_end_section ? i : i0) = i3;
                                return tag.is_end_section;
                            }
                        }
                        tag.is_standalone = true;
                    }
                    if (!tag.is_standalone)
                        text = boost::string_ref(i0, i2 - i0);
                    else if (auto partial = get<ast::partial>(&attr))
                        partial->indent.assign(i1, i2 - i1);
                    i0 = i;
                    return i == e || tag.is_end_section;
                }
                else
                {
                    pure = false;
                    ++i;
                }
            }
        }
        text = boost::string_ref(i0, i - i0);
        return true;
    }

    template<class I>
    void parse_contents
    (
        I i0, I& i, I e, delim& d, bool& pure,
        ast::content_list& attr, boost::string_ref const& section
    )
    {
        for (;;)
        {
            boost::string_ref text;
            ast::content a;
            auto end = parse_content(i0, i, e, d, pure, text, a, section);
            if (!text.empty())
                attr.push_back(text);
            if (!is_null(a))
                attr.push_back(std::move(a));
            if (end)
                return;
        }
    }

    template<class I>
    inline void parse_start(I& i, I e, ast::content_list& attr)
    {
        delim d("{{", "}}");
        bool pure = true;
        parse_contents(i, i, e, d, pure, attr, {});
    }
}}}

namespace bustache
{
    static char const* get_error_string(error_type err)
    {
        switch (err)
        {
        case error_set_delim:
            return "format_error(error_set_delim): mismatched '='";
        case error_baddelim:
            return "format_error(error_baddelim): invalid delimiter";
        case error_delim:
            return "format_error(error_delim): mismatched delimiter";
        case error_section:
            return "format_error(error_section): mismatched end section tag";
        case error_badkey:
            return "format_error(error_badkey): invalid key";
        default:
            return "format_error";
        }
    }

    format_error::format_error(error_type err)
      : runtime_error(get_error_string(err)), _err(err)
    {}

    void format::init(char const* begin, char const* end)
    {
        parser::parse_start(begin, end, _contents);
    }

    struct accum_size
    {
        using result_type = std::size_t;

        std::size_t operator()(ast::text const& text) const
        {
            return text.size();
        }

        std::size_t operator()(ast::section const& section) const
        {
            std::size_t n = 0;
            for (auto const& content : section.contents)
                n += visit(*this, content);
            return n;
        }

        template <typename T>
        std::size_t operator()(T const&) const
        {
            return 0;
        }
    };

    std::size_t format::text_size() const
    {
        accum_size accum;
        std::size_t n = 0;
        for (auto const& content : _contents)
            n += visit(accum, content);
        return n;
    }

    struct copy_text_visitor
    {
        using result_type = void;

        char* data;

        void operator()(ast::text& text)
        {
            auto n = text.size();
            std::memcpy(data, text.data(), n);
            text = {data, n};
            data += n;
        }

        void operator()(ast::section& section)
        {
            for (auto& content : section.contents)
                visit(*this, content);
        }

        template <typename T>
        void operator()(T const&) const {}
    };

    void format::copy_text(std::size_t n)
    {
        _text.reset(new char[n]);
        copy_text_visitor visitor{_text.get()};
        for (auto& content : _contents)
            visit(visitor, content);
    }
}/*//////////////////////////////////////////////////////////////////////////////
    Copyright (c) 2016 Jamboree

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////*/





namespace bustache { namespace detail
{
    value::pointer content_visitor_base::resolve(std::string const& key) const
    {
        auto ki = key.begin();
        auto ke = key.end();
        if (ki == ke)
            return{};
        value::pointer pv = nullptr;
        if (*ki == '.')
        {
            if (++ki == ke)
                return cursor;
            auto k0 = ki;
            while (*ki != '.' && ++ki != ke);
            key_cache.assign(k0, ki);
            pv = find(scope->data, key_cache);
        }
        else
        {
            auto k0 = ki;
            while (ki != ke && *ki != '.') ++ki;
            key_cache.assign(k0, ki);
            pv = scope->lookup(key_cache);
        }
        if (ki == ke)
            return pv;
        if (auto obj = get<object>(pv))
        {
            auto k0 = ++ki;
            while (ki != ke)
            {
                if (*ki == '.')
                {
                    key_cache.assign(k0, ki);
                    obj = get<object>(find(*obj, key_cache));
                    if (!obj)
                        return nullptr;
                    k0 = ++ki;
                }
                else
                    ++ki;
            }
            key_cache.assign(k0, ki);
            return find(*obj, key_cache);
        }
        return nullptr;
    }
}}

namespace bustache
{
    template
    void generate_ostream
    (
        std::ostream& out, format const& fmt,
        value::view const& data, detail::any_context const& context, option_type flag
    );

    template
    void generate_string
    (
        std::string& out, format const& fmt,
        value::view const& data, detail::any_context const& context, option_type flag
    );
}