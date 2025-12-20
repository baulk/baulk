pragma once

/*
 * from https://github.com/codereport/blackbird  - Copyright (c) 2022 Conor Hoekstra
 *
 * MIT License: https://github.com/codereport/blackbird/blob/main/LICENSE
 */

#include <functional>

    namespace combinators {

    /////////////////
    // combinators //
    /////////////////

    // B (The Bluebird)
    auto _b = [](auto f, auto g) { return [=](auto x) { return f(g(x)); }; };

    // C (The Cardinal) aka `flip` in Haskell
    auto _c = [](auto f) { return [=](auto x, auto y) { return f(y, x); }; };

    // K (Kestrel)
    auto _l_ = [](auto x, auto y) { return x; };

    // KI
    auto _r_ = [](auto x, auto y) { return y; };

    // Phi (The Phoenix)
    auto _phi = [](auto f, auto g, auto h) { return [=](auto x) { return g(f(x), h(x)); }; };

    // Phi1 (The Pheasant)
    auto _phi1_ = [](auto f, auto g, auto h) { return [=](auto x, auto y) { return g(f(x, y), h(x, y)); }; };

    // Psi (The Psi Bird)
    auto _psi = [](auto f, auto g) { return [=](auto x, auto y) { return f(g(x), g(y)); }; };

    /////////////////////////////////////////////
    // more convenient binary/unary operations //
    /////////////////////////////////////////////

    auto _eq    = [](auto x) { return [x](auto y) { return x == y; }; };
    auto _eq_   = std::equal_to{};
    auto _neq_  = std::not_equal_to{};
    auto _lt    = [](auto x) { return [x](auto y) { return x > y; }; };
    auto lt_    = [](auto x) { return [x](auto y) { return y < x; }; };
    auto _lt_   = std::less{};
    auto _gte   = [](auto x) { return [x](auto y) { return x >= y; }; };
    auto _plus  = [](auto x) { return [x](auto y) { return x + y; }; };
    auto _plus_ = std::plus{};
    auto _mul   = [](auto x) { return [x](auto y) { return x * y; }; };
    auto _mul_  = std::multiplies{};
    auto _sub   = [](auto x) { return [x](auto y) { return x - y; }; };
    auto sub_   = [](auto x) { return [x](auto y) { return y - x; }; };
    auto _sub_  = std::minus{};
    auto _or_   = std::logical_or{};
    auto _and_  = std::logical_and{};
    auto _not   = std::logical_not{};
    auto _min_  = [](auto a, auto b) { return std::min(a, b); };
    auto _max_  = [](auto a, auto b) { return std::max(a, b); };
    auto _fst   = [](auto t) { return std::get<0>(t); };
    auto _snd   = [](auto t) { return std::get<1>(t); };

} // namespace combinators
