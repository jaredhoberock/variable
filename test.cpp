#include "variable.hpp"
#include <cassert>
#include <fmt/core.h>

int main()
{
  variable<"foo"> foo;
  static_assert(foo.name == "foo");

  // this prints "foo" to the terminal
  fmt::print("{}\n", foo);

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
  }

  // XXX this fails with a reasonable static_assert
//  {
//    binding<"bar"> b{13};
//    environment env(b);
//    evaluate(foo, env);
//  }

  return 0;
}

