#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <iostream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace detail
{

template<std::size_t I, class... Ts>
constexpr auto remove_tuple_element(const std::tuple<Ts...>& t)
{
  auto helper = [&]<size_t... Is>(std::index_sequence<Is...>)
  {
    return std::make_tuple(std::get<Is + (Is >= I)>(t)...);
  };

  return helper(std::make_index_sequence<sizeof...(Ts) - 1>{});
}

// sl: abbreviation of string_literal
template<auto len>
struct sl
{
  constexpr sl(const char (&str)[len])
  {
    std::copy_n(str, len, value);
  }

  constexpr operator std::string_view() const
  {
    return {value};
  }

  char value[len];
};

} // end detail


template<detail::sl n, class T = int>
struct binding
{
  constexpr static std::string_view name = n;
  using value_type = T;
  value_type value;
};


template<class... Bindings>
class environment
{
  public:
    using tuple_type = std::tuple<Bindings...>;

    environment() = default;

    environment(const environment&) = default;

    constexpr environment(const std::tuple<Bindings...>& bindings)
      : bindings_{bindings}
    {}

    constexpr environment(const Bindings&... bindings)
      : environment{std::make_tuple(bindings...)}
    {}

    constexpr static std::size_t size()
    {
      return std::tuple_size_v<tuple_type>;
    }

    template<detail::sl name>
    constexpr static bool contains()
    {
      return find<name>() < size();
    }

    template<detail::sl name>
    constexpr decltype(auto) get() const
    {
      if constexpr (contains<name>())
      {
        auto&& binding = std::get<find<name>()>(bindings_);
        return binding.value;
      }
      else
      {
        static_assert("Name not in environment.");
        return;
      }
    }

    template<detail::sl name>
    friend constexpr decltype(auto) get(const environment& env)
    {
      return env.template get<name>();
    }

    template<detail::sl name>
    constexpr auto erase() const
    {
      if constexpr (contains<name>())
      {
        constexpr std::size_t i = find<name>();
        std::tuple new_bindings = detail::remove_tuple_element<i>(bindings_);
        return make_environment(new_bindings);
      }
      else
      {
        static_assert("Name not in environment.");
        return;
      }
    }

    template<detail::sl name, class T>
    constexpr auto set(const T& value) const
    {
      if constexpr (contains<name>())
      {
        return erase<name>().template set<name>(value);
      }
      else
      {
        binding<name,T> additional_binding{value};
        std::tuple new_bindings = std::tuple_cat(bindings_, std::tuple(additional_binding));
        return make_environment(new_bindings);
      }
    }

    template<detail::sl name, class T>
    friend constexpr auto set(const environment& self, const T& value)
    {
      return self.template set<name>(value);
    }

  private:
    template<class... OtherBindings>
    constexpr static environment<OtherBindings...> make_environment(const std::tuple<OtherBindings...>& bindings)
    {
      return {bindings};
    }

    template<detail::sl name, std::size_t i>
    constexpr static int find_beginning_at()
    {
      if constexpr (i == size())
      {
        return i;
      }
      else if constexpr(name == std::tuple_element_t<i,tuple_type>::name)
      {
        return i;
      }
      else
      {
        return find_beginning_at<name, i+1>();
      }
    }

    template<detail::sl name>
    constexpr static int find()
    {
      // XXX if we kept the Bindings... in sorted order,
      //     we could do a logarithmic search
      return find_beginning_at<name,0>();
    }

    std::tuple<Bindings...> bindings_;
};


template<class T>
concept unevaluated = requires
{
  typename T::is_unevaluated;
};

template<class L, class R>
concept at_least_one_unevaluated =
  unevaluated<L>
  or unevaluated<R>
;

template<class T>
struct evaluated_t_impl
{
  using type = T;
};

template<unevaluated T>
struct evaluated_t_impl<T>
{
  using type = typename T::value_type;
};

template<class T>
using evaluated_t = typename evaluated_t_impl<T>::type;


// evaluating something that is not an unevaluated is just the identity
template<class T, class... Bindings>
  requires (not unevaluated<T>)
constexpr T evaluate(const T& value, const environment<Bindings...>&)
{
  // XXX when T is a tuple_like, we need to map evaluate across the tuple's elements
  return value;
}


template<unevaluated E, std::invocable<evaluated_t<E>> F>
struct op1
{
  struct is_unevaluated {};
  using value_type = std::invoke_result_t<F, evaluated_t<E>>;

  template<class... Bindings>
  friend constexpr auto evaluate(const op1& self, const environment<Bindings...>& env)
  {
    return self.f(evaluate(self.expr, env));
  }

  E expr;
  F f;
};

template<class L, class R, std::invocable<evaluated_t<L>, evaluated_t<R>> F>
  requires at_least_one_unevaluated<L,R>
struct op2
{
  struct is_unevaluated {};
  using value_type = std::invoke_result_t<F,evaluated_t<L>,evaluated_t<R>>;

  template<class... Bindings>
  friend constexpr auto evaluate(const op2& self, const environment<Bindings...>& env)
  {
    return self.f(evaluate(self.lhs, env), evaluate(self.rhs, env));
  }

  L lhs;
  R rhs;
  F f;
};

template<detail::sl n, class T = int>
struct variable
{
  struct is_unevaluated {};
  constexpr static std::string_view name = n.value;
  using value_type = T;

  friend std::ostream& operator<<(std::ostream& os, variable self)
  {
    // XXX use n.value because circle has troble with simply using name
    return os << n.value;
  }

  template<class... Bindings>
  friend constexpr auto evaluate(const variable&, const environment<Bindings...>& env)
  {
    constexpr bool found = environment<Bindings...>::template contains<n>();

    if constexpr (found)
    {
      return get<n>(env);
    }
    else
    {
      static_assert(found, "evaluate(variable,env): variable name not found in environment.");
      return;
    }
  }
};

struct unary_plus
{
  constexpr auto operator()(const auto& value) const
  {
    return +value;
  }
};

template<unevaluated E>
  requires requires(evaluated_t<E> value) { +value; }
constexpr op1<E,unary_plus> operator+(const E& expr)
{
  return {expr, unary_plus()};
}

template<unevaluated E>
  requires requires(evaluated_t<E> value) { -value; }
constexpr op1<E,std::negate<>> operator-(const E& expr)
{
  return {expr, std::negate()};
}

template<unevaluated E>
  requires requires(evaluated_t<E> value) { ~value; }
constexpr op1<E,std::bit_not<>> operator~(const E& expr)
{
  return {expr, std::bit_not()};
}

template<class L, at_least_one_unevaluated<L> R>
  requires requires(evaluated_t<L> lhs, evaluated_t<R> rhs) { lhs + rhs; }
constexpr op2<L,R,std::plus<>> operator+(const L& lhs, const R& rhs)
{
  return {lhs, rhs, std::plus()};
}

template<class L, at_least_one_unevaluated<L> R>
  requires requires(evaluated_t<L> lhs, evaluated_t<R> rhs) { lhs - rhs; }
constexpr op2<L,R,std::minus<>> operator-(const L& lhs, const R& rhs)
{
  return {lhs, rhs, std::minus()};
}

template<class L, at_least_one_unevaluated<L> R>
  requires requires(evaluated_t<L> lhs, evaluated_t<R> rhs) { lhs * rhs; }
constexpr op2<L,R,std::multiplies<>> operator*(const L& lhs, const R& rhs)
{
  return {lhs, rhs, std::multiplies()};
}

template<class L, at_least_one_unevaluated<L> R>
  requires requires(evaluated_t<L> lhs, evaluated_t<R> rhs) { lhs / rhs; }
constexpr op2<L,R,std::divides<>> operator/(const L& lhs, const R& rhs)
{
  return {lhs, rhs, std::divides()};
}

template<class L, at_least_one_unevaluated<L> R>
  requires requires(evaluated_t<L> lhs, evaluated_t<R> rhs) { lhs % rhs; }
constexpr op2<L,R,std::modulus<>> operator%(const L& lhs, const R& rhs)
{
  return {lhs, rhs, std::modulus()};
}

#if defined(__cpp_user_defined_literals)

// user-defined literal operator allows variable written as literals, For example,
//
//     auto var = "block_size"_v;
//
// var has type variable<"block_size",int>.
template<detail::sl name>
constexpr variable<name> operator""_v() noexcept
{
  return {};
}

#endif // __cpp_user_defined_literals

#if __has_include(<fmt/format.h>)

namespace detail
{

template<typename T, template<typename...> class Template>
struct is_instantiation_of : std::false_type {};

template<template<typename...> class Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};

template<typename T, template<typename...> class Template>
inline constexpr bool is_instantiation_of_v = is_instantiation_of<T,Template>::value;

} // end detail

#include <fmt/format.h>

#if defined(__circle_lang__)
template<auto len, detail::sl<len> name>
struct fmt::formatter<variable<name>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const variable<name>&, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", name);
  }
};
#else
template<detail::sl name>
struct fmt::formatter<variable<name>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const variable<name>& var, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", var.name);
  }
};
#endif

template<unevaluated E, std::invocable<evaluated_t<E>> F>
struct fmt::formatter<op1<E,F>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const op1<E,F>& expr, FormatContext& ctx)
  {
    char op = '?';
    if constexpr (std::same_as<F,unary_plus>)
    {
      op = '+';
    }
    else if constexpr (std::same_as<F,std::negate<>>)
    {
      op = '-';
    }
    else if constexpr (std::same_as<F,std::bit_not<>>)
    {
      op = '~';
    }

    constexpr bool needs_parens = ::detail::is_instantiation_of_v<E,op1> or ::detail::is_instantiation_of_v<E,op2>;
    constexpr auto format_string = needs_parens ? "{}({})" : "{}{}";

    return fmt::format_to(ctx.out(), format_string, op, expr.expr);
  }
};

template<class L, class R, std::invocable<evaluated_t<L>, evaluated_t<R>> F>
struct fmt::formatter<op2<L,R,F>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const op2<L,R,F>& expr, FormatContext& ctx)
  {
    char op = '?';
    if constexpr (std::same_as<F,std::plus<>>)
    {
      op = '+';
    }
    else if constexpr (std::same_as<F,std::minus<>>)
    {
      op = '-';
    }
    else if constexpr (std::same_as<F,std::multiplies<>>)
    {
      op = '*';
    }
    else if constexpr (std::same_as<F,std::divides<>>)
    {
      op = '/';
    }
    else if constexpr (std::same_as<F,std::modulus<>>)
    {
      op = '%';
    }

    constexpr bool lhs_needs_parens = ::detail::is_instantiation_of_v<L,op1> or ::detail::is_instantiation_of_v<L,op2>;
    constexpr bool rhs_needs_parens = ::detail::is_instantiation_of_v<R,op1> or ::detail::is_instantiation_of_v<R,op2>;

    constexpr auto format_string = 
      (lhs_needs_parens and rhs_needs_parens)         ? "({}){}({})" :
      (lhs_needs_parens and not rhs_needs_parens)     ? "({}){}{}"   :
      (not lhs_needs_parens and rhs_needs_parens)     ? "{}{}({})"
                                                      : "{}{}{}"
    ;

    return fmt::format_to(ctx.out(), format_string, expr.lhs, op, expr.rhs);
  }
};

#endif // __has_include
