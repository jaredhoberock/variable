#include "unevaluated.hpp"
#include <cassert>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>

template<class N, class D>
constexpr auto ceil_div(N n, D d)
{
  return (n + d - 1) / d;
}

int main()
{
  using namespace fmt;

  constexpr auto foo = "foo"_v;
  static_assert(foo.name == "foo");

  {
    environment env{ {"foo", 13} };

    {
      auto expr = foo;
      assert("foo" == format("{}", expr));
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
      auto bar = "bar"_v;
      auto expr = foo + bar;
      assert("foo+bar" == format("{}", expr));

      auto new_env = env;
      new_env["bar"] = 7;
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
      auto bar = "bar"_v;
      auto expr = foo - bar;
      assert("foo-bar" == format("{}", expr));

      auto new_env = env;
      new_env["bar"] = 7;
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
      auto bar = "bar"_v;
      auto expr = foo * bar;
      assert("foo*bar" == format("{}", expr));

      auto new_env = env;
      new_env["bar"] = 7;
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
      auto bar = "bar"_v;
      auto expr = foo / bar;
      assert("foo/bar" == format("{}", expr));

      auto new_env = env;
      new_env["bar"] = 7;
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
      auto bar = "bar"_v;
      auto expr = foo / bar;
      assert("foo/bar" == format("{}", expr));

      auto new_env = env;
      new_env["bar"] = 7;
      assert((13 / 7) == evaluate(expr, new_env));
    }

    {
      auto expr = ceil_div(12345, "block_size"_v);
      assert("((12345+block_size)-1)/block_size" == format("{}", expr));

      auto new_env = env;
      new_env["block_size"] = 128;
      assert(((12345+128-1)/128) == evaluate(expr, new_env));
    }

    {
      auto num_blocks = ceil_div(12345, "block_size"_v);
      std::tuple shape("block_size"_v, num_blocks);
      assert("(block_size, ((12345+block_size)-1)/block_size)" == format("{}", shape));

      auto new_env = env;
      new_env["block_size"] = 128;
      assert(std::tuple(128, (12345+128-1)/128) == evaluate(shape, new_env));
    }

    {
      try
      {
        auto no_binding = "no_binding"_v;

        evaluate(no_binding, env);
        assert(false);
      }
      catch(std::runtime_error)
      {
      }
    }

    {
      variable<double> number{"number"};
      environment env{ {"number", "string"} };

      try
      {
        evaluate(number, env);
        assert(false);
      }
      catch(std::bad_any_cast)
      {
      }
    }
  }

  // this prints "foo" to the terminal
  std::cout << foo << std::endl;

  // this prints "foo" to the terminal
  std::cout << "foo"_v << std::endl;

  // this prints "foo" to the terminal
  print("{}\n", foo);

  // this prints "foo" to the terminal
  print("{}\n", "foo"_v);


  print("OK\n");

  return 0;
}

