#ifndef gtl_soa_hpp_
#define gtl_soa_hpp_

// ---------------------------------------------------------------------------
//  Copyright (c) 2022, Gregory Popovitch - greg7mdp@gmail.com
//  modified from the Mark Liu's version - licenses below
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------
// Copyright (c) 2018 Mark Liu
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ---------------------------------------------------------------------------

#include <algorithm>
#include <cassert>
#include <iostream>
#include <ostream>
#include <tuple>
#include <vector>
#include <gtl/bit_vector.hpp>

namespace gtl {

template<typename... Ts>
class soa {
public:
    using storage_type = std::tuple<std::vector<Ts>...>;

    template<size_t col_idx>
    using nth_col_type = typename std::tuple_element<col_idx, storage_type>::type;

    template<size_t col_idx>
    using col_type = typename nth_col_type<col_idx>::value_type;

    template<size_t col_idx>
    const auto& get_column() const {
        return std::get<col_idx>(data_);
    }

    template<size_t col_idx>
    nth_col_type<col_idx>& get_column() {
        return std::get<col_idx>(data_);
    }

    size_t size() const { return get_column<0>().size(); }

    bool empty() const { return get_column<0>().empty(); }

    template<typename... Xs>
    void insert(Xs... xs) {
        insert_impl(std::index_sequence_for<Ts...>{}, std::forward_as_tuple(xs...));
    }

    auto operator[](size_t idx) const {
        return std::apply([=](auto&... x) { return std::tie(x[idx]...); }, data_);
    }

    auto operator[](size_t idx) {
        return std::apply([=](auto&... x) { return std::tie(x[idx]...); }, data_);
    }

    template<size_t... I>
    auto view(size_t row) const {
        return get_row_impl(std::integer_sequence<size_t, I...>{}, row);
    }

    template<size_t... I>
    auto view(size_t row) {
        return get_row_impl(std::integer_sequence<size_t, I...>{}, row);
    }

    void clear() {
        std::apply([](auto&... x) { (x.clear(), ...); }, data_);
    }

    void resize(size_t sz) {
        std::apply([=](auto&... x) { (x.resize(sz), ...); }, data_);
    }

    void reserve(size_t sz) {
        std::apply([=](auto&... x) { (x.reserve(sz), ...); }, data_);
    }

    template<size_t col_idx, typename C>
    void sort_by_field(C&& comp) {
        size_t                 num_elems = size();
        thread_local sort_data sort_tmp; // thread_local makes it static

        sort_tmp.resize(num_elems);
        for (size_t i = 0; i < num_elems; ++i)
            sort_tmp.o[i] = i;

        auto& col = get_column<col_idx>();

        auto comp_wrapper = [&](size_t a, size_t b) { return comp(col[a], col[b]); };

        std::stable_sort(sort_tmp.o.begin(), sort_tmp.o.end(), comp_wrapper);

        sort_by_reference_impl(sort_tmp, std::index_sequence_for<Ts...>{});
    }

    template<size_t col_idx>
    void sort_by_field() {
        sort_by_field<col_idx>([](auto&& a, auto&& b) { return a < b; });
    }

    void print(std::basic_ostream<char>& ss) const {
        size_t num_elems = size();
        ss << "soa {\n";
        for (size_t i = 0; i < num_elems; ++i) {
            const auto t = (*this)[i];
            ss << "\t";
            print(ss, t, std::make_index_sequence<sizeof...(Ts)>());
            ss << '\n';
        }
        ss << "}" << '\n';
    }

private:
    struct sort_data {
        std::vector<size_t> o; // sort order
        bit_vector          done;
        void                resize(size_t sz) {
            o.resize(sz);
            done.resize(sz);
        }
    };

    template<class TupType, size_t... I>
    static void print(std::basic_ostream<char>& ss, const TupType& _tup, std::index_sequence<I...>) {
        // c++17 unary left fold, of the form `... op pack`, where `op` is the comma operator
        (..., (ss << (I == 0 ? "" : ", ") << std::get<I>(_tup)));
    }

    template<typename T, size_t... I>
    void insert_impl(std::integer_sequence<size_t, I...>, T t) {
        ((get_column<I>().push_back(std::get<I>(t))), ...);
    }

    template<size_t... I>
    auto get_row_impl(std::integer_sequence<size_t, I...>, size_t row) const {
        return std::tie(get_column<I>()[row]...);
    }

    template<size_t... I>
    auto get_row_impl(std::integer_sequence<size_t, I...>, size_t row) {
        return std::tie(get_column<I>()[row]...);
    }

    template<size_t... I>
    void sort_by_reference_impl(sort_data& sort_tmp, std::integer_sequence<size_t, I...>) {
        ((sort_col_by_reference(sort_tmp, std::integral_constant<size_t, I>{})), ...);
    }

    // o contains the index containing the value going at this position
    // so if o[0] = 5, it means that c[5] should go to c[0].
    // c contains the values to be reordered according to o.
    template<class C>
    void reorder(C& c, const std::vector<size_t>& o, bit_vector& done) {
        size_t num_elems = o.size();

        done.reset();

        for (size_t i = 0; i < num_elems; ++i) {
            if (!done[i]) {
                size_t                 curidx = i;
                typename C::value_type ci(std::move(c[i]));

                do {
                    assert(!done[curidx]);
                    done.set(curidx);
                    if (o[curidx] == i) {
                        c[curidx] = std::move(ci);
                        break;
                    }
                    c[curidx] = std::move(c[o[curidx]]);
                    curidx    = o[curidx];
                } while (o[curidx] != curidx);
            }
        }
    }

    template<size_t col_idx>
    void sort_col_by_reference(sort_data& sort_tmp, std::integral_constant<size_t, col_idx>) {
        auto& col = std::get<col_idx>(data_);
        reorder(col, sort_tmp.o, sort_tmp.done);
    }

    storage_type data_;
};

template<typename... Ts>
std::ostream& operator<<(std::ostream& cout, const gtl::soa<Ts...>& soa) {
    soa.print(cout);
    return cout;
}
}

#endif /* gtl_soa_hpp_ */
