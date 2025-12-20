#ifndef vec_utils_hpp_guard_
#define vec_utils_hpp_guard_

// ---------------------------------------------------------------------------
// Copyright (c) 2022, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------
#include <concepts>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// Some utilities to more easily code recursive algorithms using std::vector
// when you are not concerned with performance, and miss the simplicity
// of python lists
// ---------------------------------------------------------------------------

namespace gtl {

template<class T>
concept VectorLike = requires(T v) {
    /* v.reserve(1); */
    v.begin();
    v.end();
    (void)v[0];
};

// ----------------------------------------------------------------------------------
// returns a new vector which is the concatenation of the vectors passed as arguments
// ----------------------------------------------------------------------------------
template<VectorLike... Vs>
auto cat(Vs&&... vs) {
    std::common_type_t<Vs...> res;
    res.reserve((0 + ... + vs.size()));
    (..., (res.insert(res.end(), std::begin(std::forward<Vs>(vs)), std::end(std::forward<Vs>(vs)))));
    return res;
}

// -------------------------------------------------------------------------------
// implements python-like slicing for vectors, negative indices start from the end
// -------------------------------------------------------------------------------
template<VectorLike V>
auto slice(V&& v, int first = 0, int last = -1, int stride = 1) {
    std::remove_const_t<std::remove_reference_t<V>> res;
    auto                                            first_iter =
        (first >= 0 ? std::begin(std::forward<V>(v)) + first : std::end(std::forward<V>(v)) + (first + 1));
    auto last_iter = (last >= 0 ? std::begin(std::forward<V>(v)) + last : std::end(std::forward<V>(v)) + (last + 1));
    if (last_iter > first_iter) {
        std::size_t cnt = (last_iter - first_iter) / stride;
        res.reserve(cnt);
        for (; cnt-- != 0; first_iter += stride)
            res.push_back(*first_iter);
    }
    return res;
}

// ---------------------------------------------------------------------------------------
// apply a unary function to every element of a vector, returning the vector of results
// ---------------------------------------------------------------------------------------
template<class F, class T, class A, template<class, class> class V>
#ifndef _LIBCPP_VERSION // until this is available
    requires std::invocable<F&, T>
#endif
auto map(F&& f, const V<T, A>& v) {
    using result_type = std::invoke_result_t<F, T>;
    V<result_type, std::allocator<result_type>> res;
    res.reserve(v.size());
    for (const auto& x : v)
        res.push_back(std::forward<F>(f)(x));
    return res;
}

} // namespace gtl

#endif // vec_utils_hpp_guard_
