#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <string_view>
#include <tuple>
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
template<auto N>
struct sl
{
  constexpr sl(const char (&str)[N])
  {
    std::copy_n(str, N, value);
  }

  constexpr operator std::string_view() const
  {
    return {value};
  }

  char value[N];
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
      return find_beginning_at<name,0>();
    }

    std::tuple<Bindings...> bindings_;
};

// XXX it might better to name the concept or type "unevaluated"
//     we don't want to allow the operator overloads to participate unless at least one of the arguments is a variable or a op1 or op2
//     the operators should be "hidden" friends, not free functions

// XXX an expression is something that can do evaluate(expr, environment)
//     an expression should also report its type via T::value_type
template<class T>
concept expression = true;

template<expression E>
using expression_value_t = typename E::value_type;


template<detail::sl n, class T = int>
struct variable
{
  constexpr static std::string_view name = n.value;
  using value_type = T;

  template<class... Bindings>
  friend constexpr auto evaluate(variable, environment<Bindings...> env)
  {
    if constexpr (env.template contains<n>())
    {
      return env.template get<n>();
    }
    else
    {
      static_assert(env.template contains<n>(), "evaluate(variable,env): variable name not found in environment.");
      return;
    }
  }
};


template<expression E, std::invocable<expression_value_t<E>> F>
struct op1
{
  using value_type = std::invoke_result_t<F, expression_value_t<E>>;

  template<class... Bindings>
  friend constexpr auto evaluate(op1 self, environment<Bindings...> env)
  {
    return self.op(evaluate(self.expr, env));
  }

  E expr;
  F op;
};

struct unary_plus
{
  template<class A>
    requires requires (A&& arg) { +std::forward<A>(arg); }
  constexpr decltype(auto) operator()(A&& arg) const
  {
    return +std::forward<A>(arg);
  }
};

template<expression E>
  requires requires(expression_value_t<E> value) { +value; }
constexpr op1<E,unary_plus> operator+(E expr)
{
  return {expr, unary_plus()};
}

template<expression E>
  requires requires(expression_value_t<E> value) { -value; }
constexpr op1<E, std::negate<>> operator-(E expr)
{
  return {expr, std::negate()};
}

template<expression L, expression R, std::invocable<expression_value_t<L>, expression_value_t<R>> F>
struct op2
{
  using value_type = std::invoke_result_t<F, expression_value_t<L>, expression_value_t<R>>;

  template<class... Bindings>
  friend constexpr auto evaluate(op2 self, environment<Bindings...> env)
  {
    return self.op(evaluate(self.lhs, env), evaluate(self.rhs, env));
  }

  L lhs;
  R rhs;
  F op;
};

// XXX this needs a requires(expression_value_t<L> l, expression_value_t<R> r) { l OP r; }
#define EXPRESSION_BINARY_OP(OP, FUNCTOR)\
  template<expression L, expression R>\
  constexpr op2<L,R,std::FUNCTOR<>> operator OP (L lhs, R rhs) { return {lhs, rhs, std::FUNCTOR()}; }

EXPRESSION_BINARY_OP(+, plus);
EXPRESSION_BINARY_OP(-, minus);
EXPRESSION_BINARY_OP(*, multiplies);
EXPRESSION_BINARY_OP(/, divides);
EXPRESSION_BINARY_OP(%, modulus);

#undef EXPRESSION_BINARY_OP

// user-defined literal operator allows variable written as literals, For example,
//
//     auto var = "block_size"_v;
//
// var has type variable<"block_size",int>.
template<detail::sl n>
constexpr variable<n> operator""_v()
{
  return {};
}

#if __has_include(<fmt/format.h>)

#include <fmt/format.h>

template<detail::sl n>
struct fmt::formatter<variable<n>>
{
  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const variable<n>& var, FormatContext& ctx)
  {
    return fmt::format_to(ctx.out(), "{}", var.name);
  }
};

template<expression E, std::invocable<expression_value_t<E>> F>
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

    return fmt::format_to(ctx.out(), "{}{}", op, expr.expr);
  }
};

template<expression L, expression R, std::invocable<expression_value_t<L>, expression_value_t<R>> F>
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

    return fmt::format_to(ctx.out(), "{}{}{}", expr.lhs, op, expr.rhs);
  }
};

#endif // __has_include
