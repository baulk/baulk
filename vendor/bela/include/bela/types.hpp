///
#ifndef BELA_TYPES_HPP
#define BELA_TYPES_HPP
#include <cstddef>
#include <concepts>

namespace bela {
#ifndef __BELA__SSIZE_DEFINED_T
#define __BELA__SSIZE_DEFINED_T
using __bela__ssize_t = std::ptrdiff_t;
#endif
using ssize_t = __bela__ssize_t;

template <class _Ty, class... _Types>
constexpr bool is_any_of_v = // true if and only if _Ty is in _Types
    std::disjunction_v<std::is_same<_Ty, _Types>...>;

template <class T>
constexpr bool is_character_v =
    is_any_of_v<std::remove_cv_t<T>, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t>;

template <class T> constexpr bool is_u16_character_v = is_any_of_v<std::remove_cv_t<T>, wchar_t, char16_t>;

template <class T> constexpr bool is_u32_character_v = is_any_of_v<std::remove_cv_t<T>, char32_t>;

template <class T>
constexpr bool is_u8_character_v = is_any_of_v<std::remove_cv_t<T>, char, signed char, unsigned char, char8_t>;

template <class T>
constexpr bool is_strict_signed_integral_v = is_any_of_v<std::remove_cv_t<T>, short, int, long, long long>;
template <class T>
constexpr bool is_strict_unsigned_integral_v =
    is_any_of_v<std::remove_cv_t<T>, unsigned short, unsigned int, unsigned long, unsigned long long>;
template <class T>
concept strict_signed_integral = is_strict_signed_integral_v<T>;
template <class T>
concept strict_unsigned_integral = is_strict_unsigned_integral_v<T>;

template <class T>
concept character = is_character_v<T>;

template <class T>
concept not_character = !is_character_v<T>;

template <class T>
concept u32_character = is_u32_character_v<T>;

template <class T>
concept u16_character = is_u16_character_v<T>;

template <class T>
concept u8_character = is_u8_character_v<T>;

template <class T>
concept standard_layout = std::is_standard_layout_v<T>;

template <class T>
concept trivial = std::is_trivial_v<T>;

template <class T>
concept integral_superset = std::integral<T> || std::is_enum_v<T>;

template <typename E>
requires std::is_enum_v<E>
constexpr auto integral_cast(E e) { return static_cast<std::underlying_type_t<E>>(e); }

template <typename T, typename K>
requires integral_superset<T> && integral_superset<K>
constexpr bool FlagIsTrue(T a, K b) { return (a & static_cast<T>(b)) != 0; }

constexpr int64_t SizeUnInitialized{-1};

} // namespace bela

#endif