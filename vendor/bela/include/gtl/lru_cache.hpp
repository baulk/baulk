#ifndef gtl_lru_cache_hpp_
#define gtl_lru_cache_hpp_

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

namespace gtl {

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
template<class K,
         class V,
         size_t N    = 4,
         class Hash  = gtl::Hash<K>,
         class Eq    = std::equal_to<K>,
         class Mutex = std::mutex>
class lru_cache_impl {
public:
    using key_type    = K;
    using result_type = V;
    using value_type  = typename std::pair<const key_type, result_type>;

    using list_type = std::list<value_type>;
    using list_iter = typename list_type::iterator;

    using map_type = gtl::parallel_flat_hash_map<K,
                                                 list_iter,
                                                 Hash,
                                                 Eq,
                                                 std::allocator<std::pair<const key_type, list_iter>>,
                                                 N,
                                                 Mutex,
                                                 list_type>;

    static constexpr size_t num_submaps = map_type::subcnt();

    // because the cache is sharded (multiple submaps and sublists)
    // the max_size is an approximation.
    // ------------------------------------------------------------
    lru_cache_impl(size_t max_size = 65536) {
        reserve(max_size);
        set_cache_size(max_size);
        assert(_max_size > 2);
    }

    bool exists(const K& k) {
        return _cache.if_contains(k, [&](const auto&, list_type&) {});
    }

    std::optional<result_type> get(const K& k) {
        if (result_type res; _cache.if_contains(k, [&](const auto& v, list_type& l) {
                res = v.second->second;
                l.splice(l.begin(), l, v.second);
            }))
            return { res };
        return std::nullopt;
    }

    template<class Val>
    void insert(const K& key, Val&& value) {
        _cache.lazy_emplace_l(
            key,
            [&](typename map_type::value_type& v, list_type& l) {
                // called only when key was already present
                v.second->second = std::forward<Val>(value);
                l.splice(l.begin(), l, v.second);
            },
            [&](const typename map_type::constructor& ctor, list_type& l) {
                // construct value_type in place when key not present
                l.push_front(value_type(key, std::forward<Val>(value)));
                ctor(key, l.begin());
                if (l.size() > _max_size) {
                    // remove oldest
                    auto last = l.end();
                    last--;
                    auto to_delete = std::move(last->first);
                    l.pop_back();
                    return std::optional<key_type>{ to_delete };
                }
                return std::optional<key_type>{};
            });
    }

    void   clear() { _cache.clear(); }
    void   reserve(size_t n) { _cache.reserve(size_t(n * 1.1f)); }
    void   set_cache_size(size_t max_size) { _max_size = max_size / num_submaps; }
    size_t size() const { return _cache.size(); }

private:
    size_t   _max_size;
    map_type _cache;
};

// ------------------------------------------------------------------------------
// test lru_cache_test.cpp will only pass if N==0 as it checks exact numbers
// of items remaining in (or expunged from) cache.
//
// but it is fine to use a larger N for real-life applications, although it is
// mostly useful in multi threaded contexts (when using std::mutex).
// ------------------------------------------------------------------------------
template<class K, class V, class Hash = gtl::Hash<K>, class Eq = std::equal_to<K>>
using lru_cache = lru_cache_impl<K, V, 0, Hash, Eq, gtl::NullMutex>;

// ------------------------------------------------------------------------------
// uses std::mutex by default
// ------------------------------------------------------------------------------
template<class K, class V, class Hash = gtl::Hash<K>, class Eq = std::equal_to<K>>
using mt_lru_cache = lru_cache_impl<K, V, 6, Hash, Eq, std::mutex>;

} // namespace gtl

#endif // gtl_lru_cache_hpp_
