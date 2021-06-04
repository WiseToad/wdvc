#ifndef TYPETRAITS_H
#define TYPETRAITS_H

#include <type_traits>

template <typename T>
struct is_char: std::integral_constant<bool,
    std::is_same<T, char>::value ||
    std::is_same<T, wchar_t>::value ||
    std::is_same<T, char16_t>::value ||
    std::is_same<T, char32_t>::value> {};

template <typename T>
struct is_bool: std::integral_constant<bool,
    std::is_same<T, bool>::value> {};

template <typename T>
struct is_numeric: std::integral_constant<bool,
    (std::is_integral<T>::value ||
     std::is_floating_point<T>::value) &&
    !is_char<T>::value &&
    !is_bool<T>::value> {};

#endif // TYPETRAITS_H
