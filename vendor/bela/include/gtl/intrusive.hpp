#ifndef gtl_intrusive_hpp_guard_
#define gtl_intrusive_hpp_guard_

// ---------------------------------------------------------------------------
//  Copyright (c) 2022, Gregory Popovitch - greg7mdp@gmail.com
//  modified from the boost version - licenses below
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
// ---------------------------------------------------------------------------
//  Copyright (c) 2001, 2002 Peter Dimov
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
// ---------------------------------------------------------------------------
//  Copyright Andrey Semashev 2007 - 2013.
//  Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
// ---------------------------------------------------------------------------

#include <atomic>
#include <cassert>
#include <cstddef>
#include <iostream>

namespace gtl {

// ---------------------------------------------------------------------------
//  intrusive_ptr
//  -------------
//
//  A smart pointer that uses intrusive reference counting.
//
//  The object pointed to must implement the following member functions.
//      void intrusive_ptr_add_ref(T* p);
//      void intrusive_ptr_release(T* p);
//
//  These can be implemented by inheriting from gtl::intrusive_ref_counter.
//
//  The object pointed to is responsible for destroying itself when its
//  reference counts is 0.
// ---------------------------------------------------------------------------
template<class T>
class intrusive_ptr {
private:
    using this_type = intrusive_ptr;

public:
    using element_type = T;

    template<class U>
    friend class intrusive_ptr;

    constexpr intrusive_ptr() noexcept
        : px(nullptr) {}

    intrusive_ptr(T* p, bool add_ref = true) noexcept
        : px(p) {
        if (px != nullptr && add_ref)
            intrusive_ptr_add_ref(px);
    }

    template<class U> //, typename std::enable_if_t<std::is_convertible_v<U,T>, int> = 0>
    intrusive_ptr(intrusive_ptr<U> const& rhs) noexcept
        : px(rhs.get()) {
        if (px)
            intrusive_ptr_add_ref(px);
    }

    template<class U, typename std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
    intrusive_ptr(intrusive_ptr<U>&& rhs)
        : px(rhs.px) {
        rhs.px = nullptr;
    }

    intrusive_ptr(intrusive_ptr const& rhs) noexcept
        : px(rhs.px) {
        if (px)
            intrusive_ptr_add_ref(px);
    }

    intrusive_ptr(intrusive_ptr&& rhs) noexcept
        : px(rhs.px) {
        rhs.px = nullptr;
    }

    ~intrusive_ptr() {
        if (px)
            intrusive_ptr_release(px);
    }

    intrusive_ptr& operator=(intrusive_ptr const& rhs) {
        if (this != &rhs)
            this_type(rhs).swap(*this);
        return *this;
    }

    template<class U>
    intrusive_ptr& operator=(intrusive_ptr<U> const& rhs) {
        if (this != &rhs)
            this_type(rhs).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept {
        if (this != &rhs)
            this_type(std::move(rhs)).swap(*this);
        return *this;
    }

    template<class U>
    intrusive_ptr& operator=(intrusive_ptr<U>&& rhs) noexcept {
        this_type(std::move(rhs)).swap(*this);
        return *this;
    }

    intrusive_ptr& operator=(T* rhs) {
        if (rhs != px)
            this_type(rhs).swap(*this);
        return *this;
    }

    void reset() { this_type().swap(*this); }

    void reset(T* rhs) { this_type(rhs).swap(*this); }

    void reset(T* rhs, bool add_ref) { this_type(rhs, add_ref).swap(*this); }

    T* get() const noexcept { return px; }

    T* detach() noexcept {
        T* ret = px;
        px     = nullptr;
        return ret;
    }

    T& operator*() const {
        assert(px);
        return *px;
    }

    T* operator->() const {
        assert(px);
        return px;
    }

    explicit operator bool() const noexcept { return px != nullptr; }

    void swap(intrusive_ptr& rhs) noexcept {
        T* tmp = px;
        px     = rhs.px;
        rhs.px = tmp;
    }

private:
    T* px;
};

// comparison operators
// --------------------
template<class T, class U>
inline bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
    return a.get() == b.get();
}

template<class T, class U>
inline bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept {
    return a.get() != b.get();
}

template<class T, class U>
inline bool operator==(intrusive_ptr<T> const& a, U* b) noexcept {
    return a.get() == b;
}

template<class T, class U>
inline bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept {
    return a.get() != b;
}

template<class T, class U>
inline bool operator==(T* a, intrusive_ptr<U> const& b) noexcept {
    return a == b.get();
}

template<class T, class U>
inline bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept {
    return a != b.get();
}

template<class T>
inline bool operator==(intrusive_ptr<T> const& p, std::nullptr_t) noexcept {
    return p.get() == nullptr;
}

template<class T>
inline bool operator==(std::nullptr_t, intrusive_ptr<T> const& p) noexcept {
    return p.get() == nullptr;
}

template<class T>
inline bool operator!=(intrusive_ptr<T> const& p, std::nullptr_t) noexcept {
    return p.get() != nullptr;
}

template<class T>
inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const& p) noexcept {
    return p.get() != nullptr;
}

template<class T>
inline bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) noexcept {
    return std::less<T*>()(a.get(), b.get());
}

// swap
// ----
template<class T>
void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

// mem_fn support
// --------------
template<class T>
T* get_pointer(intrusive_ptr<T> const& p) noexcept {
    return p.get();
}

// pointer casts
// -------------
template<class T, class U>
intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p) {
    return static_cast<T*>(p.get());
}

template<class T, class U>
intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p) {
    return const_cast<T*>(p.get());
}

template<class T, class U>
intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p) {
    return dynamic_cast<T*>(p.get());
}

template<class T, class U>
intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U>&& p) noexcept {
    return intrusive_ptr<T>(static_cast<T*>(p.detach()), false);
}

template<class T, class U>
intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U>&& p) noexcept {
    return intrusive_ptr<T>(const_cast<T*>(p.detach()), false);
}

template<class T, class U>
intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U>&& p) noexcept {
    T* p2 = dynamic_cast<T*>(p.get());

    intrusive_ptr<T> r(p2, false);

    if (p2)
        p.detach();

    return r;
}

// operator<<
// ----------
template<class Y>
std::ostream& operator<<(std::ostream& os, intrusive_ptr<Y> const& p) {
    os << p.get();
    return os;
}

// hash_value
// ----------
template<class T>
struct hash;

template<class T>
std::size_t hash_value(intrusive_ptr<T> const& p) noexcept {
    return std::hash<T*>()(p.get());
}

} // namespace gtl

// std::hash
// ---------
namespace std {

template<class T>
struct hash<::gtl::intrusive_ptr<T>> {
    std::size_t operator()(::gtl::intrusive_ptr<T> const& p) const noexcept { return std::hash<T*>()(p.get()); }
};

} // namespace std

namespace gtl {

// --------------------------------------------------------------------------------
// \brief Thread unsafe reference counter policy for \c intrusive_ref_counter
//
// The policy instructs the \c intrusive_ref_counter base class to implement
// a reference counter suitable for single threaded use only. Pointers to the same
// object with this kind of reference counter must not be used by different threads.
// --------------------------------------------------------------------------------
struct thread_unsafe_counter {
    using type = unsigned int;

    static unsigned int load(type const& counter) noexcept { return counter; }

    static void increment(type& counter) noexcept { ++counter; }

    static unsigned int decrement(type& counter) noexcept { return --counter; }
};

// --------------------------------------------------------------------------------
// \brief Thread safe reference counter policy for \c intrusive_ref_counter
//
// The policy instructs the \c intrusive_ref_counter base class to implement
// a thread-safe reference counter, if the target platform supports multithreading.
// --------------------------------------------------------------------------------
struct thread_safe_counter {
    using type = std::atomic<unsigned int>;

    static unsigned int load(type const& counter) noexcept { return counter.load(); }

    static void increment(type& counter) noexcept { ++counter; }

    static unsigned int decrement(type& counter) noexcept { return --counter; }
};

// --------------------------------------------------------------------------------
// \brief A reference counter base class
//
// This base class can be used with user-defined classes to add support
// for \c intrusive_ptr. The class contains a reference counter defined by the
// \c CounterPolicyT.
// Upon releasing the last \c intrusive_ptr referencing the object
// derived from the \c intrusive_ref_counter class, operator \c delete
// is automatically called on the pointer to the object.
//
// The other template parameter, \c DerivedT, is the user's class that derives from
// \c intrusive_ref_counter.
// --------------------------------------------------------------------------------
template<typename DerivedT, typename CounterPolicyT>
class intrusive_ref_counter {
private:
    using counter_type = typename CounterPolicyT::type;

    mutable counter_type _refcount;

public:
    intrusive_ref_counter() noexcept
        : _refcount(0) {}

    intrusive_ref_counter(intrusive_ref_counter const&) noexcept
        : _refcount(0) {}

    intrusive_ref_counter& operator=(intrusive_ref_counter const&) noexcept {
        return *this; // refcount not modified
    }

    unsigned int use_count() const noexcept { return CounterPolicyT::load(_refcount); }

protected:
    ~intrusive_ref_counter() = default;

    friend void intrusive_ptr_add_ref(const DerivedT* p) noexcept { CounterPolicyT::increment(p->_refcount); }

    friend void intrusive_ptr_release(const DerivedT* p) noexcept {
        if (CounterPolicyT::decrement(p->_refcount) == 0)
            delete static_cast<const DerivedT*>(p);
    }
};

} // namespace gtl

#endif // gtl_intrusive_hpp_guard_
