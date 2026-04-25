#ifndef FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_
#define FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_

#include "core/include/logging.hpp"
#include "core/include/type_traits.hpp"

#include <string>
#include <type_traits>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace fade {

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
        fade::Log<fade::LogLevel::kWarning>("InputArchive Load not implemented for type.");
        return false;
    }
};

template <typename T>
concept InputArchiveType = std::derived_from<T, InputArchive>;

template <typename ArchiveType, typename T>
concept SerializableObjectType = std::is_class_v<T> && requires(ArchiveType& archive, T& obj)
{
    // The Serialize function must be defined for the type T
    { Serialize(archive, obj) };
};

// Specialization for pointers, which we don't support
template <InputArchiveType ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T*> in_name_objptr_pair)
{
    static_assert(false, "InputArchive does not support raw pointer serialization. Please use smart pointers");
    return in_archive;
}

// Specialization for smart pointers, we do support. 
// Though this support should probably somehow be implemented as an additional concept/requirement for the other function that calls LoadObject.
template <InputArchiveType ArchiveType, typename T>
    requires (SerializableObjectType<ArchiveType, T>)
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<std::unique_ptr<T>> in_name_objectptr_pair)
{
    *in_name_objectptr_pair.second = std::move(in_archive.template LoadUniquePtrObject<T>(in_name_objectptr_pair.first));
    return in_archive;
}

// Specialization for arrays
template <InputArchiveType ArchiveType, VectorType T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_name_array_pair)
{
    in_archive.LoadArray(in_name_array_pair.first, *in_name_array_pair.second);
    return in_archive;
}

// Specialization for most types
template <InputArchiveType ArchiveType, typename T>
    requires (!SerializableObjectType<ArchiveType, T> && !VectorType<T>)
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_name_value_pair) 
{
    in_archive.Load(in_name_value_pair.first, *in_name_value_pair.second);
    return in_archive;
}

// Specialization for serializable objects
template <InputArchiveType ArchiveType, typename T>
    requires (SerializableObjectType<ArchiveType, T>)
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_name_obj_pair) 
{
    in_archive.LoadObject(in_name_obj_pair.first, *in_name_obj_pair.second);
    return in_archive;
}

} // namespace fade

#endif // FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_
