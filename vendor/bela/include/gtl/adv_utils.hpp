#ifndef gtl_adv_utils_hpp_guard
#define gtl_adv_utils_hpp_guard

// ---------------------------------------------------------------------------
// Copyright (c) 2023, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------

#include <concepts>
#include <cstdint>
#include <numeric>
#include <optional>
#include <utility>
#include <gtl/utils.hpp>

namespace gtl {

// ---------------------------------------------------------------------------
// Generalized binary search
// inspired by the fun post from Brent Yorgey.
// https://byorgey.wordpress.com/2023/01/01/competitive-programming-in-haskell-better-binary-search/
// also see: https://julesjacobs.com/notes/binarysearch/binarysearch.pdf
// ---------------------------------------------------------------------------

// Concept for "Middle" function.
// compute a potential next value to look at, given the current interval. When
// mid l r returns a value, it must be strictly in between l and r
// ---------------------------------------------------------------------------
template<class T, class MiddleFn>
concept MiddlePredicate = requires(MiddleFn fn, T l, T r) {
    {
        fn(l, r)
    } -> std::same_as<std::optional<T>>;
};

// Concept for a Boolean predicate function
//
template<class T, class BoolPredFn>
concept BooleanPredicate = requires(BoolPredFn fn, T val) {
    {
        fn(val)
    } -> std::convertible_to<bool>;
};

// returns a pair<T, T> such that pred(first) == false and pred(second) == true
// in the interval [l, r], pred should switch from false to true exactly once.
template<class T, class Pred, class Middle>
std::pair<T, T> binary_search(Middle&& middle, Pred&& pred, T l, T r)
    requires MiddlePredicate<T, Middle> && BooleanPredicate<T, Pred>
{
    assert(std::forward<Pred>(pred)(l) == false && std::forward<Pred>(pred)(r) == true);
    auto m = std::forward<Middle>(middle)(l, r);
    if (!m)
        return { l, r };
    return std::forward<Pred>(pred)(*m)
               ? binary_search<T, Pred, Middle>(std::forward<Middle>(middle), std::forward<Pred>(pred), l, *m)
               : binary_search<T, Pred, Middle>(std::forward<Middle>(middle), std::forward<Pred>(pred), *m, r);
}

// ---------------------------------------------------------------------------
// Some middle functions
// ---------------------------------------------------------------------------
template<class T>
concept is_double = std::same_as<T, double>;

// We stop when l and r are exactly one apart
// ------------------------------------------
template<class T>
    requires std::integral<T> || is_double<T>
struct middle {};

template<std::integral T>
struct middle<T> {
    std::optional<T> operator()(T l, T r) {
        if (r - l > 1)
            return std::midpoint(l, r);
        return {};
    }
};

// Compare doubles using the binary representation
// -----------------------------------------------
template<is_double T>
struct middle<T> {
    std::optional<T> operator()(T l, T r) {
        uint64_t* il = reinterpret_cast<uint64_t*>(&l);
        uint64_t* ir = reinterpret_cast<uint64_t*>(&r);
        auto      m  = middle<uint64_t>()(*il, *ir);
        if (m) {
            uint64_t med = *m;
            return *(T*)&med;
        }
        return {};
    }
};

} // namespace gtl

#endif // gtl_adv_utils_hpp_guard
