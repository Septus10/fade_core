#ifndef FADE_CORE_REFLECTION_REFLECT_ENUM_HPP_
#define FADE_CORE_REFLECTION_REFLECT_ENUM_HPP_

#include <string>

namespace fade {

template <typename T>
T StringToEnum(const std::string& in_string);

template <typename T>
std::string EnumToString(const T& in_enum);

}

#endif // FADE_CORE_REFLECTION_REFLECT_ENUM_HPP_