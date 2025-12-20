#ifndef gtl_memoize_hpp_
#define gtl_memoize_hpp_

// ---------------------------------------------------------------------------
// Copyright (c) 2022, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------

#include <cassert>
#include <cstddef>
#include <gtl/phmap.hpp>
#include <list>
#include <optional>
#include <tuple>

namespace gtl {

// ------------------------------------------------------------------------------
// deduct the variadic parameter pack of a lambda.
// see https://stackoverflow.com/questions/71630906 for alternate method
// ------------------------------------------------------------------------------
template<typename... Ts>
struct pack {};

template<class>
struct pack_helper;

template<class R, class... Args>
struct pack_helper<R (*)(Args...)> {
    using args = pack<Args...>;
};

// ---- for lambdas
template<class T>
struct pack_helper : public pack_helper<decltype(&T::operator())> {};

template<class LambdaClass, class R, class... Args>
struct pack_helper<R (LambdaClass::*)(Args...) const> {
    using args = pack<Args...>;
};

template<class F>
using this_pack_helper = typename pack_helper<F>::args;

// ------------------------------------------------------------------------------
// Given a callable object (often a function), this class provides a new
// callable which either invokes the original one, caching the returned value,
// or returns the cached returned value if the arguments match a previous call.
// Of course this should be used only for pure functions without side effects.
//
// if a mutex (such as std::mutex) is provided, this callable object can be
// used safely from multiple threads without any additional locking.
//
// This version keeps all unique results in the hash map.
//
// recursive specifies if the function to be memoized is recursive.
// Using the default value of true is always safe, but setting it
// to false should be a little faster for non-recursive functions.
//
// the size_t N parameter configures the number of submaps as a power of 2, so
// N=6 create 64 submaps. Each submap has its own mutex to reduce contention
// in a heavily multithreaded context.
//
// see example: examples/memoize/mt_memoize.cpp
// ------------------------------------------------------------------------------
template<class F, bool recursive = true, size_t N = 6, class Mutex = std::mutex, class = this_pack_helper<F>>
class mt_memoize;

template<class F, bool recursive, size_t N, class Mutex, class... Args>
class mt_memoize<F, recursive, N, Mutex, pack<Args...>> {
public:
    using key_type    = std::tuple<Args...>;
    using result_type = decltype(std::declval<F>()(std::declval<Args>()...));
    using map_type    = gtl::parallel_flat_hash_map<key_type,
                                                 result_type,
                                                 gtl::Hash<key_type>,
                                                 std::equal_to<key_type>,
                                                 std::allocator<std::pair<const key_type, result_type>>,
                                                 N,
                                                 Mutex>;

    mt_memoize(F&& f)
        : _f(std::move(f)) {}

    mt_memoize(F& f)
        : _f(f) {}

    std::optional<result_type> contains(Args... args) {
        key_type key(args...);
        if (result_type res; _cache.if_contains(key, [&](const auto& v) { res = v.second; }))
            return { res };
        return {};
    }

    result_type operator()(Args... args) {
        key_type key(args...);
        if constexpr (!std::is_same_v<Mutex, gtl::NullMutex> && !recursive) {
            // because we are using a mutex, we must be in a multithreaded context,
            // so use lazy_emplace_l to take the lock only once.
            // --------------------------------------------------------------------
            result_type res;
            _cache.lazy_emplace_l(
                key,
                [&](typename map_type::value_type& v) {
                    // called only when key was already present
                    res = v.second;
                },
                [&](const typename map_type::constructor& ctor) {
                    // construct value_type in place when key not present
                    res = _f(args...);
                    ctor(key, res);
                });
            return res;
        } else {
            // nullmutex or recursive function -> use two API for allowing more
            // recursion and preventing deadlocks, as _f is called outside the
            // hashmap APIs.
            // --------------------------------------------------------------
            result_type res;
            if (_cache.if_contains(key, [&](const auto& v) { res = v.second; }))
                return res;
            res = _f(args...);
            _cache.emplace(key, res);
            return res;
        }
    }

    void   clear() { _cache.clear(); }
    void   reserve(size_t n) { _cache.reserve(n); }
    size_t size() const { return _cache.size(); }

private:
    F        _f;
    map_type _cache;
};

// ------------------------------------------------------------------------------
// Given a callable object (often a function), this class provides a new
// callable which either invokes the original one, caching the returned value,
// or returns the cached returned value if the arguments match a previous call.
// Of course this should be used only for pure functions without side effects.
//
// This version keeps all unique results in the hash map.
//
// when the memoized function is used from a single thread, use the gtl::NullMutex
// so we don't incur any locking cost.
// ------------------------------------------------------------------------------
template<class F, size_t N = 4>
using memoize = mt_memoize<F, true, N, gtl::NullMutex>;

// ------------------------------------------------------------------------------
// Given a callable object (often a function), this class provides a new
// callable which either invokes the original one, caching the returned value,
// or returns the cached returned value if the arguments match a previous call.
// Of course this should be used only for pure functions without side effects.
//
// if a mutex (such as std::mutex) is provided, this callable object can be
// used safely from multiple threads without any additional locking.
//
// This version keeps all unique results in the hash map.
//
// the size_t N parameter configures the number of submaps as a power of 2, so
// N=6 create 64 submaps. Each submap has its own mutex to reduce contention
// in a heavily multithreaded context.
//
// see example: examples/memoize/mt_memoize_lru.cpp
//
// ------------------------------------------------------------------------------
template<class F, size_t N = 6, class Mutex = std::mutex, class = this_pack_helper<F>>
class mt_memoize_lru;

template<class F, size_t N, class Mutex, class... Args>
class mt_memoize_lru<F, N, Mutex, pack<Args...>> {
public:
    using key_type    = std::tuple<Args...>;
    using result_type = decltype(std::declval<F>()(std::declval<Args>()...));
    using value_type  = typename std::pair<const key_type, result_type>;

    using list_type = std::list<value_type>;
    using list_iter = typename list_type::iterator;

    using map_type = gtl::parallel_flat_hash_map<key_type,
                                                 list_iter,
                                                 gtl::Hash<key_type>,
                                                 std::equal_to<key_type>,
                                                 std::allocator<std::pair<const key_type, list_iter>>,
                                                 N,
                                                 Mutex,
                                                 list_type>;

    static constexpr size_t num_submaps = map_type::subcnt();

    mt_memoize_lru(F&& f, size_t max_size = 65536)
        : _f(std::move(f)) {
        reserve(max_size);
        set_cache_size(max_size);
        assert(_max_size > 2);
    }

    mt_memoize_lru(F& f, size_t max_size = 65536)
        : _f(f) {
        reserve(max_size);
        set_cache_size(max_size);
        assert(_max_size > 2);
    }

    std::optional<result_type> contains(Args... args) {
        key_type key(args...);
        if (result_type res; _cache.if_contains(key, [&](const auto& v, list_type&) { res = v.second->second; }))
            return { res };
        return {};
    }

    result_type operator()(Args... args) {
        key_type    key(args...);
        result_type res;
        _cache.lazy_emplace_l(
            key,
            [&](typename map_type::value_type& v, list_type& l) {
                // called only when key was already present
                res = v.second->second;
                l.splice(l.begin(), l, v.second);
            },
            [&](const typename map_type::constructor& ctor, list_type& l) {
                // construct value_type in place when key not present
                res = _f(args...);
                l.push_front(value_type(key, res));
                ctor(key, l.begin());
                if (l.size() >= _max_size) {
                    // remove oldest
                    auto last = l.end();
                    last--;
                    auto to_delete = std::move(last->first);
                    l.pop_back();
                    return std::optional<key_type>{ to_delete };
                }
                return std::optional<key_type>{};
            });
        return res;
    }

    void   clear() { _cache.clear(); }
    void   reserve(size_t n) { _cache.reserve(size_t(n * 1.1f)); }
    void   set_cache_size(size_t max_size) { _max_size = max_size / num_submaps; }
    size_t size() const { return _cache.size(); }

private:
    F        _f;
    size_t   _max_size;
    map_type _cache;
};

// ------------------------------------------------------------------------------
// when the memoized function is used from a single thread, use the gtl::NullMutex
// so we don't incur any locking cost.
// ------------------------------------------------------------------------------
template<class F, size_t N = 4>
using memoize_lru = mt_memoize_lru<F, N, gtl::NullMutex>;

// ------------------------------------------------------------------------------
// Attempting to create a lazy list... not ready for prime time :-)
// ------------------------------------------------------------------------------
template<class T, class F>
class lazy_list {
public:
    lazy_list(T first, F next)
        : _first(std::move(first))
        , _next(std::move(next)) {}

#if 1
    // wrong implementation, because it calls _next instead of the memoized version
    const T& operator[](size_t idx) const {
        if (idx == 0)
            return _first;
        return _next(this, idx);
    }
#else
    // can't get this to build unfortunately
    const T& operator[](size_t idx) const {
        auto _memoized_next{ gtl::memoize<decltype(_next)>(_next) };
        if (idx == 0)
            return _first;
        return _memoized_next(this, idx);
    }
#endif

private:
    using mem_next = T (*)(size_t idx, const T& first, F&& next);
    // using memo_t = decltype(std::declval< gtl::memoize<F>(std::declval<F>()) >());

#if 0
    template <class F>
    static T logify_recursion(F &f, size_t start, size_t end) {
        // untested!
        if (start == 0) {
            auto hit = f.contains(end);
            if (hit)
                return *hit;
        }

        size_t width = end - start;
        if (width > 64) {
            (void)f(start + width / 2);
            return logify_recursion(f, start + width / 2, end);
        }

        return f(end);
    }
#endif

    T _first;
    F _next;
};

} // namespace gtl

#endif // gtl_memoize_hpp_
