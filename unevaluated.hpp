#pragma once

#include <any>
#include <concepts>
#include <fmt/core.h>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

using environment = std::map<std::string, std::any, std::less<>>;

// evaluating any old value is just the identity
template<class T>
constexpr T evaluate(const T& value, const environment& env)
{
  return value;
}

// evaluating a tuple returns a tuple of the recursive evaluation of its elements
template<class... Ts>
constexpr auto evaluate(const std::tuple<Ts...>& t, const environment& env)
{
  return std::apply([&](const auto&... elements)
  {
    return std::make_tuple(evaluate(elements, env)...);
  },
  t);
}

template<class T>
using evaluated_t = decltype(evaluate(std::declval<T>(), std::declval<environment>()));

template<class T, class U>
concept not_same_as = not std::same_as<T,U>;

// a type is unevaluated if evaluating an instance would yield a different type
template<class T>
concept unevaluated = requires(T val, environment env)
{
  { evaluate(val, env) } -> not_same_as<T>;
};

template<class L, class R>
concept at_least_one_unevaluated =
  unevaluated<L>
  or unevaluated<R>
;

template<class T>
struct variable
{
  std::string_view name;

  friend T evaluate(const variable& self, const environment& env)
  {
    auto found = env.find(self.name);
    if(found == env.end()) throw std::runtime_error(fmt::format("{} not found in env", self.name));
    return std::any_cast<T>(found->second);
  }

  friend std::ostream& operator<<(std::ostream& os, const variable& self)
  {
    return os << self.name;
  }
};

template<unevaluated E, std::invocable<evaluated_t<E>> F>
struct op1
{
  friend auto evaluate(const op1& self, const environment& env)
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
  friend auto evaluate(const op2& self, const environment& env)
  {
    return self.f(evaluate(self.lhs,env), evaluate(self.rhs,env));
  }

  L lhs;
  R rhs;
  F f;
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

// user-defined literal operator allows variable written as literals, For example,
//
//     auto var = "block_size"_v;
//
// var has type variable<int>.
constexpr variable<int> operator""_v(const char* str, std::size_t n) noexcept
{
  return {std::string_view{str,n}};
}

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

template<class T>
struct fmt::formatter<variable<T>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const variable<T>& var, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", var.name);
  }
};

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

