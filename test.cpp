#include "variable.hpp"
#include <cassert>
#include <fmt/core.h>
#include <iostream>

int main()
{
  using namespace fmt;

  variable<"foo"> foo;
  static_assert(foo.name == "foo");

  {
    binding<"foo"> b{13};
    environment env(b);

    {
      auto expr = foo;
      assert(13 == evaluate(foo, env));
    }

    {
      auto expr = +foo;
      assert("+foo" == format("{}", expr));
      assert(+13 == evaluate(expr, env));
    }

    {
      auto expr = -foo;
      assert("-foo" == format("{}", expr));
      assert(-13 == evaluate(expr, env));
    }

    {
      auto expr = ~foo;
      assert("~foo" == format("{}", expr));
      assert(~13 == evaluate(expr, env));
    }

    {
      auto expr = 7 + foo;
      assert("7+foo" == format("{}", expr));
      assert(7+13 == evaluate(expr, env));
    }

    {
      auto expr = foo + 7;
      assert("foo+7" == format("{}", expr));
      assert(13 + 7 == evaluate(expr, env));
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo + bar;
      assert("foo+bar" == format("{}", expr));
      assert((13 + 7) == evaluate(expr, new_env));
    }

    {
      auto expr = foo - 7;
      assert("foo-7" == format("{}", expr));
      assert((13 - 7) == evaluate(expr, env));
    }

    {
      auto expr = 7 - foo;
      assert("7-foo" == format("{}", expr));
      assert((7 - 13) == evaluate(expr, env));
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo - bar;
      assert("foo-bar" == format("{}", expr));
      assert((13 - 7) == evaluate(expr, new_env));
    }

    {
      auto expr = foo * 7;
      assert("foo*7" == format("{}", expr));
      assert((13 * 7) == evaluate(expr, env));
    }

    {
      auto expr = 7 * foo;
      assert("7*foo" == format("{}", expr));
      assert((7 * 13) == evaluate(expr, env));
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo * bar;
      assert("foo*bar" == format("{}", expr));
      assert((13 * 7) == evaluate(expr, new_env));
    }

    {
      auto expr = foo / 7;
      assert("foo/7" == format("{}", expr));
      assert((13 / 7) == evaluate(expr, env));
    }

    {
      auto expr = 7 / foo;
      assert("7/foo" == format("{}", expr));
      assert((7 / 13) == evaluate(expr, env));
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo / bar;
      assert("foo/bar" == format("{}", expr));
      assert((13 / 7) == evaluate(expr, new_env));
    }

    {
      auto expr = foo % 7;
      assert("foo%7" == format("{}", expr));
      assert((13 % 7) == evaluate(expr, env));
    }

    {
      auto expr = 7 / foo;
      assert("7/foo" == format("{}", expr));
      assert((7 / 13) == evaluate(expr, env));
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo / bar;
      assert("foo/bar" == format("{}", expr));
      assert((13 / 7) == evaluate(expr, new_env));
    }
  }

  // this prints "foo" to the terminal
  std::cout << foo << std::endl;

  // this prints "foo" to the terminal
  std::cout << "foo"_v << std::endl;

  // this prints "foo" to the terminal
  fmt::print("{}\n", foo);

  // this prints "foo" to the terminal
  fmt::print("{}\n", "foo"_v);

  // this fails with a reasonable static_assert
//  {
//    binding<"bar"> b{13};
//    environment env(b);
//    evaluate(foo, env);
//  }

  return 0;
}

