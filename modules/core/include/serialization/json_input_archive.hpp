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

    virtual void PushContext(const std::string& in_name) override
    {
        if (JsonObject* current_context = context_stack_.top(); current_context != nullptr)
        {
            if (auto it = current_context->members.find(in_name); it != current_context->members.end())
            {
                if (JsonObject* child_object = std::get_if<JsonObject>(&it->second.value); child_object != nullptr)
                {
                    context_stack_.push(child_object);
                }
                else
                {
                    fade::core::Log<fade::core::LogLevel::kWarning>("Context '{}' is not a JSON object.", in_name);
                }
            }
            else
            {
                fade::core::Log<fade::core::LogLevel::kWarning>("Context '{}' not found.", in_name);
            }
        }        
    }

    virtual void PopContext() override
    {
        if (context_stack_.size() > 1)
        {
            context_stack_.pop();
        }
        else
        {
            fade::core::Log<fade::core::LogLevel::kWarning>("Trying to pop root context.");
        }
    }

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
