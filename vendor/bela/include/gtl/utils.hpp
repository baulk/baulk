#ifndef gtl_utils_hpp_guard
#define gtl_utils_hpp_guard

// ---------------------------------------------------------------------------
// Copyright (c) 2022, Gregory Popovitch - greg7mdp@gmail.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------

#include <cstdint>
#include <utility>

namespace gtl {

// ---------------------------------------------------------------------------
// An object which calls a lambda in its constructor, and another one in
// its destructor
//
// This object must be captured in a local variable, Otherwise, since it is a
// temporary, it will be destroyed immediately, thus calling the unset function.
//
//          scoped_set_unset rollback(...); // good
// ---------------------------------------------------------------------------
template<class Unset>
class scoped_set_unset {
public:
    template<class Set>
    [[nodiscard]] scoped_set_unset(Set&& set, Unset&& unset, bool do_it = true)
        : do_it_(do_it)
        , unset_(std::move(unset)) {
        if (do_it_)
            std::forward<Set>(set)();
    }

    ~scoped_set_unset() {
        if (do_it_)
            unset_();
    }

    void dismiss() noexcept { do_it_ = false; }

    scoped_set_unset(scoped_set_unset&&)                 = delete;
    scoped_set_unset(const scoped_set_unset&)            = delete;
    scoped_set_unset& operator=(const scoped_set_unset&) = delete;
    void* operator new(std::size_t)                      = delete;

private:
    bool  do_it_;
    Unset unset_;
};

// ---------------------------------------------------------------------------
// An object which calls a lambda in its  destructor
//
// This object must be captured in a local variable, Otherwise, since it is a
// temporary, it will be destroyed immediately, thus calling the function.
//
//          scoped_guard rollback(...); // good
// ---------------------------------------------------------------------------
template<class F>
class scoped_guard {
public:
    [[nodiscard]] scoped_guard(F&& unset, bool do_it = true) noexcept(std::is_nothrow_move_constructible_v<F>)
        : do_it_(do_it)
        , unset_(std::move(unset)) {}

    ~scoped_guard() {
        if (do_it_)
            unset_();
    }

    void dismiss() noexcept { do_it_ = false; }

    scoped_guard(scoped_guard&&)                 = delete;
    scoped_guard(const scoped_guard&)            = delete;
    scoped_guard& operator=(const scoped_guard&) = delete;
    void* operator new(std::size_t)              = delete;

private:
    bool do_it_;
    F    unset_;
};

#if __cpp_deduction_guides >= 201703
template<class F>
scoped_guard(F f) -> scoped_guard<F>;
#endif

// ---------------------------------------------------------------------------
// An object which assigns a value to a variable in its constructor, and resets
// the previous its destructor
//
// This object must be captured in a local variable, Otherwise, since it is a
// temporary, it will be destroyed immediately, thus reverting to the old value
// immediately
//
//        scoped_set_value rollback(retries, 7); // good
// ---------------------------------------------------------------------------
template<class T>
class scoped_set_value {
public:
    template<class V>
    [[nodiscard]] scoped_set_value(T& var, V&& val, bool do_it = true) noexcept(std::is_nothrow_copy_constructible_v<T> &&
                                                                                std::is_nothrow_move_assignable_v<T>)
        : v_(var)
        , do_it_(do_it) {
        if (do_it_) {
            old_value_ = std::move(v_);
            v_         = std::forward<V>(val);
        }
    }

    ~scoped_set_value() {
        if (do_it_)
            v_ = std::move(old_value_);
    }

    void dismiss() noexcept { do_it_ = false; }

    scoped_set_value(const scoped_set_value&)            = delete;
    scoped_set_value& operator=(const scoped_set_value&) = delete;
    scoped_set_value(scoped_set_value&&)                 = delete;
    scoped_set_value& operator=(scoped_set_value&&)      = delete;
    void* operator new(std::size_t)                      = delete;

private:
    T&   v_;
    T    old_value_;
    bool do_it_;
};

// ---------------------------------------------------------------------------
// assigns val to var, and returns true if the value changed
// ---------------------------------------------------------------------------
template<class T, class V>
bool change(T& var, V&& val) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                      std::is_nothrow_copy_assignable_v<T>) {
    if (var != val) {
        var = std::forward<V>(val);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// assigns val to var, and returns the previous value
// ---------------------------------------------------------------------------
template<class T, class V>
T replace(T& var, V&& val) noexcept(std::is_nothrow_move_assignable_v<T> &&
                                    std::is_nothrow_copy_assignable_v<T>) {
    T old = std::move(var);
    var   = std::forward<V>(val);
    return old;
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
template<class... T>
struct always_false : std::false_type {};

// ---------------------------------------------------------------------------
// A baseclass to keep track of modifications.
// Change member `x_` using `set_with_ts`
// ---------------------------------------------------------------------------
class timestamp {
public:
    [[nodiscard]] timestamp() noexcept { stamp_ = ++clock_; }

    [[nodiscard]] timestamp(uint64_t stamp) noexcept
        : stamp_(stamp) {}

    void touch() noexcept { stamp_ = ++clock_; }
    void touch(const timestamp& o) noexcept { stamp_ = o.stamp_; }

    void reset() noexcept { stamp_ = 0; }
   
    [[nodiscard]] bool is_set() const noexcept { return !!stamp_; }
    [[nodiscard]] bool is_newer_than(const timestamp& o) const noexcept { return stamp_ > o.stamp_; }
    [[nodiscard]] bool is_older_than(const timestamp& o) const noexcept { return stamp_ < o.stamp_; }

    bool operator==(const timestamp& o) const noexcept { return stamp_ == o.stamp_; }
    bool operator<(const timestamp& o) const noexcept { return stamp_ < o.stamp_; }
    bool operator>(const timestamp& o) const noexcept { return stamp_ > o.stamp_; }

    // returns most recent
    timestamp  operator|(const timestamp& o) const noexcept { return stamp_ > o.stamp_ ? stamp_ : o.stamp_; }
    timestamp& operator|=(const timestamp& o) noexcept {
        *this = *this | o;
        return *this;
    }

    [[nodiscard]] uint64_t get() const noexcept { return stamp_; }

    [[nodiscard]] timestamp get_timestamp() const noexcept { return *this; }

    template<class T, class V>
    bool set_with_ts(T& var, V&& val) {
        if (gtl::change(var, std::forward<V>(val))) {
            this->touch();
            return true;
        }
        return false;
    }

private:
    uint64_t               stamp_;
    static inline uint64_t clock_ = 0;
};

// ---------------------------------------------------------------------------
// A baseclass (using CRTP) for classes providing get_timestamp()
// ---------------------------------------------------------------------------
template<class T>
class provides_timestamp {
public:
    template<class TS>
    bool is_newer_than(const TS& o) const {
        return static_cast<const T*>(this)->get_timestamp() > o.get_timestamp();
    }

    template<class TS>
    bool is_older_than(const TS& o) const {
        return static_cast<const T*>(this)->get_timestamp() < o.get_timestamp();
    }

    // returns most recent
    template<class TS>
    timestamp operator|(const TS& o) const {
        return static_cast<const T*>(this)->get_timestamp() | o.get_timestamp();
    }
};

} // namespace gtl

#endif // gtl_utils_hpp_guard
