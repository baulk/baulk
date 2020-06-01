////////
// port absl::Span
#ifndef BELA_SPAN_HPP
#define BELA_SPAN_HPP
#pragma once
#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>
#include <string>
#include "macros.hpp"

namespace bela {
namespace span_internal {
constexpr size_t Min(size_t a, size_t b) noexcept { return a < b ? a : b; }
// Wrappers for access to container data pointers.
template <typename C>
constexpr auto GetDataImpl(C &c, char) noexcept // NOLINT(runtime/references)
    -> decltype(c.data()) {
  return c.data();
}

// Before C++17, std::string::data returns a const char* in all cases.
inline char *GetDataImpl(std::string &s, // NOLINT(runtime/references)
                         int) noexcept {
  return &s[0];
}
inline wchar_t *GetDataImpl(std::wstring &s, // NOLINT(runtime/references)
                            int) noexcept {
  return &s[0];
}

template <typename C>
constexpr auto GetData(C &c) noexcept // NOLINT(runtime/references)
    -> decltype(GetDataImpl(c, 0)) {
  return GetDataImpl(c, 0);
}

// Detection idioms for size() and data().
template <typename C>
using HasSize = std::is_integral<std::decay_t<decltype(std::declval<C &>().size())>>;

// We want to enable conversion from vector<T*> to Span<const T* const> but
// disable conversion from vector<Derived> to Span<Base>. Here we use
// the fact that U** is convertible to Q* const* if and only if Q is the same
// type or a more cv-qualified version of U.  We also decay the result type of
// data() to avoid problems with classes which have a member function data()
// which returns a reference.
template <typename T, typename C>
using HasData =
    std::is_convertible<std::decay_t<decltype(GetData(std::declval<C &>()))> *, T *const *>;

// Extracts value type from a Container
template <typename C> struct ElementType {
  using type = typename std::remove_reference_t<C>::value_type;
};

template <typename T, size_t N> struct ElementType<T (&)[N]> { using type = T; };

template <typename C> using ElementT = typename ElementType<C>::type;

template <typename T>
using EnableIfMutable = typename std::enable_if<!std::is_const<T>::value, int>::type;

template <template <typename> class SpanT, typename T> bool EqualImpl(SpanT<T> a, SpanT<T> b) {
  static_assert(std::is_const<T>::value, "");
  return std::equal(a.begin(), a.end(), b.begin(), b.end());
}

template <template <typename> class SpanT, typename T> bool LessThanImpl(SpanT<T> a, SpanT<T> b) {
  // We can't use value_type since that is remove_cv_t<T>, so we go the long way
  // around.
  static_assert(std::is_const<T>::value, "");
  return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

// The `IsConvertible` classes here are needed because of the
// `std::is_convertible` bug in libcxx when compiled with GCC. This build
// configuration is used by Android NDK toolchain. Reference link:
// https://bugs.llvm.org/show_bug.cgi?id=27538.
template <typename From, typename To> struct IsConvertibleHelper {
private:
  static std::true_type testval(To);
  static std::false_type testval(...);

public:
  using type = decltype(testval(std::declval<From>()));
};

template <typename From, typename To> struct IsConvertible : IsConvertibleHelper<From, To>::type {};

// TODO(zhangxy): replace `IsConvertible` with `std::is_convertible` once the
// older version of libcxx is not supported.
template <typename From, typename To>
using EnableIfConvertibleTo = typename std::enable_if<IsConvertible<From, To>::value>::type;
} // namespace span_internal
//
template <typename T> class Span {
private:
  // Used to determine whether a Span can be constructed from a container of
  // type C.
  template <typename C>
  using EnableIfConvertibleFrom = typename std::enable_if<span_internal::HasData<T, C>::value &&
                                                          span_internal::HasSize<C>::value>::type;

  // Used to SFINAE-enable a function when the slice elements are const.
  template <typename U>
  using EnableIfConstView = typename std::enable_if<std::is_const<T>::value, U>::type;

  // Used to SFINAE-enable a function when the slice elements are mutable.
  template <typename U>
  using EnableIfMutableView = typename std::enable_if<!std::is_const<T>::value, U>::type;

public:
  using value_type = std::remove_cv_t<T>;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  static const size_type npos = ~(size_type(0));

  constexpr Span() noexcept : Span(nullptr, 0) {}
  constexpr Span(pointer array, size_type length) noexcept : ptr_(array), len_(length) {}

  // Implicit conversion constructors
  template <size_t N>
  constexpr Span(T (&a)[N]) noexcept // NOLINT(runtime/explicit)
      : Span(a, N) {}

  // Explicit reference constructor for a mutable `Span<T>` type. Can be
  // replaced with MakeSpan() to infer the type parameter.
  template <typename V, typename = EnableIfConvertibleFrom<V>,
            typename = EnableIfMutableView<V>>
  explicit Span(V &v) noexcept // NOLINT(runtime/references)
      : Span(span_internal::GetData(v), v.size()) {}

  // Implicit reference constructor for a read-only `Span<const T>` type
  template <typename V, typename = EnableIfConvertibleFrom<V>,
            typename = EnableIfConstView<V>>
  constexpr Span(const V &v) noexcept // NOLINT(runtime/explicit)
      : Span(span_internal::GetData(v), v.size()) {}

  // Implicit constructor from an initializer list, making it possible to pass a
  // brace-enclosed initializer list to a function expecting a `Span`. Such
  // spans constructed from an initializer list must be of type `Span<const T>`.
  //
  //   void Process(absl::Span<const int> x);
  //   Process({1, 2, 3});
  //
  // Note that as always the array referenced by the span must outlive the span.
  // Since an initializer list constructor acts as if it is fed a temporary
  // array (cf. C++ standard [dcl.init.list]/5), it's safe to use this
  // constructor only when the `std::initializer_list` itself outlives the span.
  // In order to meet this requirement it's sufficient to ensure that neither
  // the span nor a copy of it is used outside of the expression in which it's
  // created:
  //
  //   // Assume that this function uses the array directly, not retaining any
  //   // copy of the span or pointer to any of its elements.
  //   void Process(absl::Span<const int> ints);
  //
  //   // Okay: the std::initializer_list<int> will reference a temporary array
  //   // that isn't destroyed until after the call to Process returns.
  //   Process({ 17, 19 });
  //
  //   // Not okay: the storage used by the std::initializer_list<int> is not
  //   // allowed to be referenced after the first line.
  //   absl::Span<const int> ints = { 17, 19 };
  //   Process(ints);
  //
  //   // Not okay for the same reason as above: even when the elements of the
  //   // initializer list expression are not temporaries the underlying array
  //   // is, so the initializer list must still outlive the span.
  //   const int foo = 17;
  //   absl::Span<const int> ints = { foo };
  //   Process(ints);
  //
  template <typename LazyT = T,
            typename = EnableIfConstView<LazyT>>
  Span(std::initializer_list<value_type> v) noexcept // NOLINT(runtime/explicit)
      : Span(v.begin(), v.size()) {}

  // Accessors

  // Span::data()
  //
  // Returns a pointer to the span's underlying array of data (which is held
  // outside the span).
  constexpr pointer data() const noexcept { return ptr_; }

  // Span::size()
  //
  // Returns the size of this span.
  constexpr size_type size() const noexcept { return len_; }

  // Span::length()
  //
  // Returns the length (size) of this span.
  constexpr size_type length() const noexcept { return size(); }

  // Span::empty()
  //
  // Returns a boolean indicating whether or not this span is considered empty.
  constexpr bool empty() const noexcept { return size() == 0; }

  // Span::operator[]
  //
  // Returns a reference to the i'th element of this span.
  constexpr reference operator[](size_type i) const noexcept {
    // MSVC 2015 accepts this as constexpr, but not ptr_[i]
    return *(data() + i);
  }

  // Span::at()
  //
  // Returns a reference to the i'th element of this span.
  constexpr reference at(size_type i) const { return *(data() + i); } // No Exception

  // Span::front()
  //
  // Returns a reference to the first element of this span.
  constexpr reference front() const noexcept { return BELA_ASSERT(size() > 0), *data(); }

  // Span::back()
  //
  // Returns a reference to the last element of this span.
  constexpr reference back() const noexcept {
    return BELA_ASSERT(size() > 0), *(data() + size() - 1);
  }

  // Span::begin()
  //
  // Returns an iterator to the first element of this span.
  constexpr iterator begin() const noexcept { return data(); }

  // Span::cbegin()
  //
  // Returns a const iterator to the first element of this span.
  constexpr const_iterator cbegin() const noexcept { return begin(); }

  // Span::end()
  //
  // Returns an iterator to the last element of this span.
  constexpr iterator end() const noexcept { return data() + size(); }

  // Span::cend()
  //
  // Returns a const iterator to the last element of this span.
  constexpr const_iterator cend() const noexcept { return end(); }

  // Span::rbegin()
  //
  // Returns a reverse iterator starting at the last element of this span.
  constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

  // Span::crbegin()
  //
  // Returns a reverse const iterator starting at the last element of this span.
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  // Span::rend()
  //
  // Returns a reverse iterator starting at the first element of this span.
  constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

  // Span::crend()
  //
  // Returns a reverse iterator starting at the first element of this span.
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // Span mutations

  // Span::remove_prefix()
  //
  // Removes the first `n` elements from the span.
  void remove_prefix(size_type n) noexcept {
    assert(size() >= n);
    ptr_ += n;
    len_ -= n;
  }

  // Span::remove_suffix()
  //
  // Removes the last `n` elements from the span.
  void remove_suffix(size_type n) noexcept {
    assert(size() >= n);
    len_ -= n;
  }

  // Span::subspan()
  //
  // Returns a `Span` starting at element `pos` and of length `len`. Both `pos`
  // and `len` are of type `size_type` and thus non-negative. Parameter `pos`
  // must be <= size(). Any `len` value that points past the end of the span
  // will be trimmed to at most size() - `pos`. A default `len` value of `npos`
  // ensures the returned subspan continues until the end of the span.
  //
  // Examples:
  //
  //   std::vector<int> vec = {10, 11, 12, 13};
  //   absl::MakeSpan(vec).subspan(1, 2);  // {11, 12}
  //   absl::MakeSpan(vec).subspan(2, 8);  // {12, 13}
  //   absl::MakeSpan(vec).subspan(1);     // {11, 12, 13}
  //   absl::MakeSpan(vec).subspan(4);     // {}
  //   absl::MakeSpan(vec).subspan(5);     // throws std::out_of_range
  constexpr Span subspan(size_type pos = 0, size_type len = npos) const {
    return (pos <= size()) ? Span(data() + pos, span_internal::Min(size() - pos, len)) : (Span());
  }

  // Span::first()
  //
  // Returns a `Span` containing first `len` elements. Parameter `len` is of
  // type `size_type` and thus non-negative. `len` value must be <= size().
  //
  // Examples:
  //
  //   std::vector<int> vec = {10, 11, 12, 13};
  //   absl::MakeSpan(vec).first(1);  // {10}
  //   absl::MakeSpan(vec).first(3);  // {10, 11, 12}
  //   absl::MakeSpan(vec).first(5);  // throws std::out_of_range
  constexpr Span first(size_type len) const {
    return (len <= size()) ? Span(data(), len) : (Span());
  }

  // Span::last()
  //
  // Returns a `Span` containing last `len` elements. Parameter `len` is of
  // type `size_type` and thus non-negative. `len` value must be <= size().
  //
  // Examples:
  //
  //   std::vector<int> vec = {10, 11, 12, 13};
  //   absl::MakeSpan(vec).last(1);  // {13}
  //   absl::MakeSpan(vec).last(3);  // {11, 12, 13}
  //   absl::MakeSpan(vec).last(5);  // throws std::out_of_range
  constexpr Span last(size_type len) const {
    return (len <= size()) ? Span(size() - len + data(), len) : (Span());
  }

private:
  pointer ptr_;
  size_type len_;
};

template <typename T> const typename Span<T>::size_type Span<T>::npos;

// Span relationals

// Equality is compared element-by-element, while ordering is lexicographical.
// We provide three overloads for each operator to cover any combination on the
// left or right hand side of mutable Span<T>, read-only Span<const T>, and
// convertible-to-read-only Span<T>.
// TODO(zhangxy): Due to MSVC overload resolution bug with partial ordering
// template functions, 5 overloads per operator is needed as a workaround. We
// should update them to 3 overloads per operator using non-deduced context like
// string_view, i.e.
// - (Span<T>, Span<T>)
// - (Span<T>, non_deduced<Span<const T>>)
// - (non_deduced<Span<const T>>, Span<T>)

// operator==
template <typename T> bool operator==(Span<T> a, Span<T> b) {
  return span_internal::EqualImpl<Span, const T>(a, b);
}
template <typename T> bool operator==(Span<const T> a, Span<T> b) {
  return span_internal::EqualImpl<Span, const T>(a, b);
}
template <typename T> bool operator==(Span<T> a, Span<const T> b) {
  return span_internal::EqualImpl<Span, const T>(a, b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator==(const U &a, Span<T> b) {
  return span_internal::EqualImpl<Span, const T>(a, b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator==(Span<T> a, const U &b) {
  return span_internal::EqualImpl<Span, const T>(a, b);
}

// operator!=
template <typename T> bool operator!=(Span<T> a, Span<T> b) { return !(a == b); }
template <typename T> bool operator!=(Span<const T> a, Span<T> b) { return !(a == b); }
template <typename T> bool operator!=(Span<T> a, Span<const T> b) { return !(a == b); }
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator!=(const U &a, Span<T> b) {
  return !(a == b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator!=(Span<T> a, const U &b) {
  return !(a == b);
}

// operator<
template <typename T> bool operator<(Span<T> a, Span<T> b) {
  return span_internal::LessThanImpl<Span, const T>(a, b);
}
template <typename T> bool operator<(Span<const T> a, Span<T> b) {
  return span_internal::LessThanImpl<Span, const T>(a, b);
}
template <typename T> bool operator<(Span<T> a, Span<const T> b) {
  return span_internal::LessThanImpl<Span, const T>(a, b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator<(const U &a, Span<T> b) {
  return span_internal::LessThanImpl<Span, const T>(a, b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator<(Span<T> a, const U &b) {
  return span_internal::LessThanImpl<Span, const T>(a, b);
}

// operator>
template <typename T> bool operator>(Span<T> a, Span<T> b) { return b < a; }
template <typename T> bool operator>(Span<const T> a, Span<T> b) { return b < a; }
template <typename T> bool operator>(Span<T> a, Span<const T> b) { return b < a; }
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator>(const U &a, Span<T> b) {
  return b < a;
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator>(Span<T> a, const U &b) {
  return b < a;
}

// operator<=
template <typename T> bool operator<=(Span<T> a, Span<T> b) { return !(b < a); }
template <typename T> bool operator<=(Span<const T> a, Span<T> b) { return !(b < a); }
template <typename T> bool operator<=(Span<T> a, Span<const T> b) { return !(b < a); }
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator<=(const U &a, Span<T> b) {
  return !(b < a);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator<=(Span<T> a, const U &b) {
  return !(b < a);
}

// operator>=
template <typename T> bool operator>=(Span<T> a, Span<T> b) { return !(a < b); }
template <typename T> bool operator>=(Span<const T> a, Span<T> b) { return !(a < b); }
template <typename T> bool operator>=(Span<T> a, Span<const T> b) { return !(a < b); }
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator>=(const U &a, Span<T> b) {
  return !(a < b);
}
template <typename T, typename U,
          typename = span_internal::EnableIfConvertibleTo<U, bela::Span<const T>>>
bool operator>=(Span<T> a, const U &b) {
  return !(a < b);
}

// MakeSpan()
//
// Constructs a mutable `Span<T>`, deducing `T` automatically from either a
// container or pointer+size.
//
// Because a read-only `Span<const T>` is implicitly constructed from container
// types regardless of whether the container itself is a const container,
// constructing mutable spans of type `Span<T>` from containers requires
// explicit constructors. The container-accepting version of `MakeSpan()`
// deduces the type of `T` by the constness of the pointer received from the
// container's `data()` member. Similarly, the pointer-accepting version returns
// a `Span<const T>` if `T` is `const`, and a `Span<T>` otherwise.
//
// Examples:
//
//   void MyRoutine(absl::Span<MyComplicatedType> a) {
//     ...
//   };
//   // my_vector is a container of non-const types
//   std::vector<MyComplicatedType> my_vector;
//
//   // Constructing a Span implicitly attempts to create a Span of type
//   // `Span<const T>`
//   MyRoutine(my_vector);                // error, type mismatch
//
//   // Explicitly constructing the Span is verbose
//   MyRoutine(absl::Span<MyComplicatedType>(my_vector));
//
//   // Use MakeSpan() to make an absl::Span<T>
//   MyRoutine(absl::MakeSpan(my_vector));
//
//   // Construct a span from an array ptr+size
//   absl::Span<T> my_span() {
//     return absl::MakeSpan(&array[0], num_elements_);
//   }
//
template <int &... ExplicitArgumentBarrier, typename T>
constexpr Span<T> MakeSpan(T *ptr, size_t size) noexcept {
  return Span<T>(ptr, size);
}

template <int &... ExplicitArgumentBarrier, typename T>
Span<T> MakeSpan(T *begin, T *end) noexcept {
  return BELA_ASSERT(begin <= end), Span<T>(begin, end - begin);
}

template <int &... ExplicitArgumentBarrier, typename C>
constexpr auto MakeSpan(C &c) noexcept // NOLINT(runtime/references)
    -> decltype(bela::MakeSpan(span_internal::GetData(c), c.size())) {
  return MakeSpan(span_internal::GetData(c), c.size());
}

template <int &... ExplicitArgumentBarrier, typename T, size_t N>
constexpr Span<T> MakeSpan(T (&array)[N]) noexcept {
  return Span<T>(array, N);
}

// MakeConstSpan()
//
// Constructs a `Span<const T>` as with `MakeSpan`, deducing `T` automatically,
// but always returning a `Span<const T>`.
//
// Examples:
//
//   void ProcessInts(absl::Span<const int> some_ints);
//
//   // Call with a pointer and size.
//   int array[3] = { 0, 0, 0 };
//   ProcessInts(absl::MakeConstSpan(&array[0], 3));
//
//   // Call with a [begin, end) pair.
//   ProcessInts(absl::MakeConstSpan(&array[0], &array[3]));
//
//   // Call directly with an array.
//   ProcessInts(absl::MakeConstSpan(array));
//
//   // Call with a contiguous container.
//   std::vector<int> some_ints = ...;
//   ProcessInts(absl::MakeConstSpan(some_ints));
//   ProcessInts(absl::MakeConstSpan(std::vector<int>{ 0, 0, 0 }));
//
template <int &... ExplicitArgumentBarrier, typename T>
constexpr Span<const T> MakeConstSpan(T *ptr, size_t size) noexcept {
  return Span<const T>(ptr, size);
}

template <int &... ExplicitArgumentBarrier, typename T>
Span<const T> MakeConstSpan(T *begin, T *end) noexcept {
  return BELA_ASSERT(begin <= end), Span<const T>(begin, end - begin);
}

template <int &... ExplicitArgumentBarrier, typename C>
constexpr auto MakeConstSpan(const C &c) noexcept -> decltype(MakeSpan(c)) {
  return MakeSpan(c);
}

template <int &... ExplicitArgumentBarrier, typename T, size_t N>
constexpr Span<const T> MakeConstSpan(const T (&array)[N]) noexcept {
  return Span<const T>(array, N);
}

} // namespace bela

#endif
