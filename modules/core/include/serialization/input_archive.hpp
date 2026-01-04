#ifndef FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_
#define FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_

#include <string>
#include <type_traits>
#include <vector>
#include <map>
#include <cstdint>

#include "core/include/logging.hpp"

namespace fade::core {

template <typename T>
using name_value_pair = std::pair<const std::string, T*>;

template <typename T>
using const_name_value_pair = std::pair<const std::string&, const T*>;

class InputArchive
{
public:
    virtual ~InputArchive() = default;

    template <typename T>
    bool Load(const std::string& in_name, T& out_variable)
    {
        fade::core::Log<fade::core::LogLevel::kWarning>("InputArchive Load not implemented for type.");
        return false;
    }
};

template <typename T>
concept IsInputArchiveClass = std::derived_from<T, InputArchive>;

template <typename ArchiveType, typename T>
concept SerializableObject = std::is_class_v<T> && requires(ArchiveType& archive, T& obj)
{
    // The Serialize function must be defined for the type T
    { Serialize(archive, obj) };
};

// Specialization for pointers, which we don't support
template <typename ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T*> in_name_objptr_pair)
    requires (IsInputArchiveClass<ArchiveType>)
{
    static_assert(false, "InputArchive does not support raw pointer serialization. Please use smart pointers");
    return in_archive;
}

// Specialization for smart pointers, we do support. 
// Though this support should probably somehow be implemented as an additional concept/requirement for the other function that calls LoadObject.
template <typename ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<std::unique_ptr<T>> in_name_objectptr_pair)
    requires (SerializableObject<ArchiveType, T> && IsInputArchiveClass<ArchiveType>)
{
    *in_name_objectptr_pair.second = std::move(in_archive.template LoadUniquePtrObject<T>(in_name_objectptr_pair.first));
    return in_archive;
}

// Specialization for most types
template <typename ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_name_value_pair) 
    requires (IsInputArchiveClass<ArchiveType> && !SerializableObject<ArchiveType, T>)
{
    in_archive.Load(in_name_value_pair.first, *in_name_value_pair.second);
    return in_archive;
}

// Specialization for serializable objects
template <typename ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_name_obj_pair) 
    requires (SerializableObject<ArchiveType, T> && IsInputArchiveClass<ArchiveType>)
{
    in_archive.LoadObject(in_name_obj_pair.first, *in_name_obj_pair.second);
    return in_archive;
}

} // namespace fade::core

#endif // FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_
