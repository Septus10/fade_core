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

    virtual void PushContext(const std::string& in_name) = 0;

    virtual void PopContext() = 0;
};

template <typename T>
concept IsInputArchiveClass = std::derived_from<T, InputArchive>;

template <typename ArchiveType, class ObjType>
bool Serialize(ArchiveType& in_output_archive, ObjType& in_obj) requires(IsInputArchiveClass<ArchiveType>);

template <typename ArchiveType, typename T>
concept SerializableObject = requires(ArchiveType& archive, T& obj)
{
    { Serialize(archive, obj) };
    std::is_class_v<T>;
};

template <typename ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_obj) 
    requires(SerializableObject<ArchiveType, T> && IsInputArchiveClass<ArchiveType>)
{
    in_archive.PushContext(in_obj.first);
    Serialize(in_archive, *in_obj.second);
    in_archive.PopContext();
    return in_archive;
}

template <typename ArchiveType>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<bool> in_name_bool_pair) 
    requires(IsInputArchiveClass<ArchiveType>)
{
    // Implementation for bool type
    in_archive.Load(in_name_bool_pair.first, *in_name_bool_pair.second);
    return in_archive;
}

template <typename ArchiveType>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<std::string> in_name_string_pair)
    requires(IsInputArchiveClass<ArchiveType>)
{
    // Implementation for string type
    in_archive.Load(in_name_string_pair.first, *in_name_string_pair.second);
    return in_archive;
}

} // namespace fade::core

#endif // FADE_CORE_SERIALIZATION_INPUT_ARCHIVE_HPP_
