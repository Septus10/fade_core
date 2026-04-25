#ifndef FADE_CORE_SERIALIZATION_SERIALIZATION_HPP
#define FADE_CORE_SERIALIZATION_SERIALIZATION_HPP

// Fade includes
#include "core/include/serialization/input_archive.hpp"
#include "core/include/serialization/output_archive.hpp"

// STL includes
#include <type_traits>

namespace fade {

// Used, for the time being, to prepare for reflection. 
// Since we would no longer require to manually write the name of the field with reflection.
#define ARCHIVE_PARAM(archive, obj, field) \
if constexpr (fade::OutputArchiveType<std::remove_reference_t<decltype(archive)>>) \
{ \
    in_archive << fade::const_name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
} \
else if constexpr (fade::InputArchiveType<std::remove_reference_t<decltype(archive)>>) \
{ \
    in_archive << fade::name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
}

}

#endif