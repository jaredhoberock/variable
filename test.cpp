#include "variable.hpp"
#include <cassert>
#include <fmt/core.h>
#include <iostream>

int main()
{
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
      assert(+13 == evaluate(expr, env));

      // this prints "+foo" to the terminal
      fmt::print("{}\n", expr);
    }

    {
      auto expr = -foo;
      assert(-13 == evaluate(expr, env));

      // this prints "-foo" to the terminal
      fmt::print("{}\n", expr);
    }

    {
      variable<"bar"> bar;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo - bar;
      assert((13 - 7) == evaluate(expr, new_env));

      // this prints "foo-bar" to the terminal
      fmt::print("{}\n", expr);
    }

    {
      auto bar = "bar"_v;
      auto new_env = set<"bar">(env, 7);

      auto expr = foo * bar;
      assert((13 * 7) == evaluate(expr, new_env));

      // this prints "foo*bar" to the terminal
      fmt::print("{}\n", expr);
    }

    {
      auto expr = foo / foo;
      assert((13 / 13) == evaluate(expr, env));

      // this prints "foo/foo" to the terminal
      fmt::print("{}\n", expr);
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

  // XXX this fails with a reasonable static_assert
//  {
//    binding<"bar"> b{13};
//    environment env(b);
//    evaluate(foo, env);
//  }

  return 0;
}

