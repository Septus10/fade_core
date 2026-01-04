#ifndef FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_
#define FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_

#include "core/include/serialization/input_archive.hpp"
#include "core/include/logging.hpp"

#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <filesystem>
#include <unordered_map>
#include <stack>
#include <cassert>
#include <memory>

namespace fade::core {

#undef NULL

enum class JsonValueType : std::uint8_t
{
    STRING,
    NUMBER,
    OBJECT,
    ARRAY,
    TRUE,
    FALSE,
    NULL
};

struct JsonArray
{
    JsonArray() = default;

    JsonArray(const JsonArray& in_other)
    {
        elements = in_other.elements;
    }
    
    JsonArray(JsonArray&& in_other) noexcept
    {
        elements = std::move(in_other.elements);
    }

    JsonArray& operator=(const JsonArray& in_other)
    {
        if (this != &in_other) {
            elements = in_other.elements;
        }
        return *this;
    }
    
    JsonArray& operator=(JsonArray&& in_other) noexcept
    {
        if (this != &in_other) {
            elements = std::move(in_other.elements);
        }
        return *this;
    }

    std::vector<struct JsonValue> elements;
};

struct JsonObject
{
    JsonObject() = default;

    JsonObject(const JsonObject& in_other)
    {
        members = in_other.members;
    }
    
    JsonObject(JsonObject&& in_other) noexcept
    {
        members = std::move(in_other.members);
    }

    JsonObject& operator=(const JsonObject& in_other)
    {
        if (this != &in_other) {
            members = in_other.members;
        }
        return *this;
    }
    
    JsonObject& operator=(JsonObject&& in_other) noexcept
    {
        if (this != &in_other) {
            members = std::move(in_other.members);
        }
        return *this;
    }

    bool IsValid() const;

    std::unordered_map<std::string, struct JsonValue> members;
};

struct JsonValue
{
    JsonValue() = default;

    JsonValue(const JsonValue& in_other) = default;
    JsonValue(JsonValue&& in_other) noexcept = default;

    JsonValue& operator=(const JsonValue& in_other) = default;
    JsonValue& operator=(JsonValue&& in_other) noexcept = default;

    std::variant<std::string, double, JsonObject, JsonArray, bool, std::nullptr_t> value;
};

template <typename T>
concept IsNumberType = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

template <typename T>
concept IsStringType = std::is_same_v<T, std::string>;

template <typename T>
concept IsObjectType = std::is_class_v<T>;

class JsonInputArchive : public InputArchive 
{
public:
    JsonInputArchive()
    {
    }

    [[nodiscard]] 
    bool Parse(const std::string& in_json_string);

    [[nodiscard]] 
    bool Parse(std::ifstream& in_json_file_stream);

    [[nodiscard]] 
    bool Parse(const std::filesystem::path& in_json_file_path);

    // std::variant<std::string, double, JsonObject, JsonArray, bool, std::nullptr_t> value;

    template <typename T>
    bool Load(const std::string& in_name, T& out_variable)
        requires(IsNumberType<T>)
    {
        return LoadInternal<double, T>(in_name, out_variable);
    }

    template <typename T>
    bool Load(const std::string& in_name, T& out_variable)
        requires(IsStringType<T>)
    {
        return LoadInternal<std::string, T>(in_name, out_variable);
    }

    template <typename T>
    bool Load(const std::string& in_name, T& out_variable)
        requires(std::is_same_v<T, bool>)
    {
        return LoadInternal<bool, T>(in_name, out_variable);
    }

    template <typename T>
    std::unique_ptr<T> LoadUniquePtrObject(const std::string& in_name)
    {
        if (JsonObject* current_context_ = context_stack_.top(); current_context_ != nullptr)
        {
            if (auto it = current_context_->members.find(in_name); it != current_context_->members.end())
            {
                if (!std::holds_alternative<std::nullptr_t>(it->second.value))
                {
                    std::unique_ptr<T> return_ptr = std::make_unique<T>();
                    // Only if the value is actually a json object should we push and pop context.
                    if (JsonObject* object_ptr = std::get_if<JsonObject>(&it->second.value); object_ptr != nullptr)
                    {
                        PushContext(object_ptr);
                        Serialize(*this, *return_ptr.get());
                        PopContext();
                    }
                    else
                    {
                        Serialize(*this, *return_ptr.get());
                    }

                    return std::move(return_ptr);
                }
            }
        }

        return nullptr;
    }

    template <typename T>
    bool LoadObject(const std::string& in_name, T& out_object)
    {
        bool success = false;

        if (JsonObject* current_context_ = context_stack_.top(); current_context_ != nullptr)
        {
            if (auto it = current_context_->members.find(in_name); it != current_context_->members.end())
            {
                // Only if the value is actually a json object should we push and pop context.
                if (JsonObject* object_ptr = std::get_if<JsonObject>(&it->second.value); object_ptr != nullptr)
                {
                    PushContext(object_ptr);
                    success |= Serialize(*this, out_object);
                    PopContext();
                }
                // Otherwise, the serialize function has its own way to serialize the value
                else if (!std::holds_alternative<std::nullptr_t>(it->second.value))
                {
                    success |= Serialize(*this, out_object);
                }
            }
        }

        return success;
    }

    /**
     * Push a new JSON object to the context stack.
     * 
     * This is used when entering a new object scope during deserialization.
     */
    void PushContext(JsonObject* in_json_object_ptr);

    /**
     * Pop the current JSON object from the context stack when finished deserializing it.
     */
    void PopContext();

private:
    template <typename VariantType, typename VariableType>
    bool LoadInternal(const std::string& in_name, VariableType& out_variable)
    {
        if (JsonObject* current_context_ = context_stack_.top(); current_context_ != nullptr)
        {
            if (auto it = current_context_->members.find(in_name); it != current_context_->members.end())
            {
                if (VariantType* value_ptr = std::get_if<VariantType>(&it->second.value); value_ptr != nullptr)
                {
                    out_variable = static_cast<VariableType>(*value_ptr);
                    return true;
                }
                else if (std::holds_alternative<std::nullptr_t>(it->second.value))
                {
                    return true;
                }
                else
                {
                    fade::core::Log<fade::core::LogLevel::kWarning>("Failed to load value for key '{}'", in_name);
                }
            }
        }

        return false;
    }

    [[nodiscard]] 
    bool ParseJsonFromStream(std::istream& in_stream);

private:
    JsonObject root_object_;

    std::stack<JsonObject*> context_stack_;
};

} // namespace fade::core

#endif // FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_
