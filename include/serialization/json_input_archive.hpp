#ifndef FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_
#define FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"
#include "core/include/serialization/input_archive.hpp"
#include "core/include/reflection/reflect_enum.hpp"
#include "core/include/logging.hpp"
#include "core/include/type_traits.hpp"

// STL includes
#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <filesystem>
#include <unordered_map>
#include <stack>
#include <cassert>
#include <memory>

namespace fade {

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
    JsonArray(const JsonArray& in_other) = default;    
    JsonArray(JsonArray&& in_other) noexcept = default;

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

    fade::DynamicArray<struct JsonValue> elements;
};

struct JsonObject
{
    JsonObject() = default;
    JsonObject(const JsonObject& in_other) = default;    
    JsonObject(JsonObject&& in_other) noexcept = default;

    JsonObject& operator=(const JsonObject& in_other) = default;
    JsonObject& operator=(JsonObject&& in_other) noexcept = default;

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

template <typename T>
concept IsEnumType = std::is_enum_v<T>;

class JsonInputArchive : public InputArchive 
{
public:
    JsonInputArchive() = default;

    [[nodiscard]] 
    bool Parse(const std::string& in_json_string);

    [[nodiscard]] 
    bool Parse(std::ifstream& in_json_file_stream);

    [[nodiscard]] 
    bool Parse(const std::filesystem::path& in_json_file_path);

    // std::variant<std::string, double, JsonObject, JsonArray, bool, std::nullptr_t> value;

    template <typename T>
    bool Load(const std::string& in_name, T& out_number)
        requires(IsNumberType<T>)
    {
        return LoadInternal<double, T>(in_name, out_number);
    }

    template <typename T>
    bool LoadFromValue(const JsonValue& in_json_value, T& out_number)
        requires(IsNumberType<T>)
    {
        return LoadFromValueInternal<double, T>(in_json_value, out_number);
    }

    template <typename T>
    bool Load(const std::string& in_name, T& out_string)
        requires(IsStringType<T>)
    {
        return LoadInternal<std::string, T>(in_name, out_string);
    }

    template <typename T>
    bool LoadFromValue(const JsonValue& in_json_value, T& out_string)
        requires(IsStringType<T>)
    {
        return LoadFromValueInternal<std::string, T>(in_json_value, out_string);
    }

    template <typename T>
    bool Load(const std::string& in_name, T& out_bool)
        requires(std::is_same_v<T, bool>)
    {
        return LoadInternal<bool, T>(in_name, out_bool);
    }

    template <typename T>
    bool LoadFromValue(const JsonValue& in_json_value, T& out_bool)
        requires(std::is_same_v<T, bool>)
    {
        return LoadFromValueInternal<bool, T>(in_json_value, out_bool);
    }

    template <typename T>
    bool Load(const std::string& in_name, T& out_enum)
        requires(IsEnumType<T>)
    {
        return LoadEnumInternal<T>(in_name, out_enum);
    }

    template <typename T>
    bool LoadFromValue(const JsonValue& in_json_value, T& out_enum)
        requires(IsEnumType<T>)
    {
        std::string enum_string { 0 };
        if (const std::string* string_ptr = std::get_if<std::string>(&in_json_value.value); string_ptr != nullptr)
        {
            enum_string = *string_ptr;
        }

        out_enum = StringToEnum<T>(enum_string);

        return enum_string.empty();
    }

    template <typename T>
    bool LoadArray(const std::string& in_name, fade::DynamicArray<T>& out_array)
    {
        return PerformFunctionOnMember([this](JsonValue& in_json_value, fade::DynamicArray<T>& out_array){
            // Only if the value is actually a json object should we push and pop context.
            if (JsonArray* array_ptr = std::get_if<JsonArray>(&in_json_value.value); array_ptr != nullptr)
            {
                fade::DynamicArray<JsonValue>& json_values = array_ptr->elements;
                out_array.reserve(json_values.size());
                for (JsonValue& json_value : json_values)
                {                
                    if constexpr (UniquePtrType<T>)
                    {
                        if (std::unique_ptr<T> element = LoadUniquePtrObjectFromValue<T>(json_value); element != nullptr)
                        {
                            out_array.push_back(std::move(element));
                        }
                    } 
                    else if constexpr (SerializableObjectType<JsonInputArchive, T>)
                    {
                        if (T element; LoadObjectFromValue<T>(json_value, element))
                        {
                            out_array.push_back(std::move(element));
                        }
                    }
                    else
                    {
                        if (T element; LoadFromValue<T>(json_value, element))
                        {
                            out_array.push_back(element);
                        }
                    }
                }
            }

            return out_array.size() > 0;
        }, in_name, out_array);
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
    std::unique_ptr<T> LoadUniquePtrObjectFromValue(JsonValue& in_json_value) 
    {
        if (!std::holds_alternative<std::nullptr_t>(in_json_value.value))
        {
            std::unique_ptr<T> return_ptr = std::make_unique<T>();
            // Only if the value is actually a json object should we push and pop context.
            if (JsonObject* object_ptr = std::get_if<JsonObject>(&in_json_value.value); object_ptr != nullptr)
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

        return std::unique_ptr<T>(nullptr);
    }

    template <typename T>
    bool LoadObject(const std::string& in_name, T& out_object)
    {
        return PerformFunctionOnMember([this](JsonValue& in_json_value, T& out_object){
            return LoadObjectFromValue(in_json_value, out_object);
        }, in_name, out_object);
    }

    template <typename T>
    bool LoadObjectFromValue(JsonValue& in_json_value, T& out_object)
    {
        bool success = false;

        // Only if the value is actually a json object should we push and pop context.
        if (JsonObject* object_ptr = std::get_if<JsonObject>(&in_json_value.value); object_ptr != nullptr)
        {
            PushContext(object_ptr);
            success |= Serialize(*this, out_object);
            PopContext();
        }
        // Otherwise, the serialize function has its own way to serialize the value
        else if (!std::holds_alternative<std::nullptr_t>(in_json_value.value))
        {
            success |= Serialize(*this, out_object);
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
    bool LoadFromValueInternal(const JsonValue& in_json_value, VariableType& out_variable)
    {
        if (const VariantType* value_ptr = std::get_if<VariantType>(&in_json_value.value); value_ptr != nullptr)
        {
            out_variable = static_cast<VariableType>(*value_ptr);
            return true;
        }
        else if (std::holds_alternative<std::nullptr_t>(in_json_value.value))
        {
            return true;
        }
        
        return false;
    }

    template <typename VariantType, typename VariableType>
    bool LoadInternal(const std::string& in_name, VariableType& out_variable)
    {
        return PerformFunctionOnMember([this](const JsonValue& in_json_value, VariableType& out_variable){
            return LoadFromValueInternal<VariantType, VariableType>(in_json_value, out_variable);
        }, in_name, out_variable);
    }

    template <typename VariableType>
    bool LoadEnumInternal(const std::string& in_name, VariableType& out_variable)
    {
        return PerformFunctionOnMember([this](const JsonValue& in_json_value, VariableType& out_variable) {
            return LoadFromValue<VariableType>(in_json_value, out_variable);
        }, in_name, out_variable);
    }

    template <typename Func, typename VariableType>
    bool PerformFunctionOnMember(Func in_func, const std::string& in_name, VariableType& out_variable)
    {
        bool success = false;
        if (JsonObject* current_context_ = context_stack_.top(); current_context_ != nullptr)
        {
            if (auto it = current_context_->members.find(in_name); it != current_context_->members.end())
            {
                success |= in_func(it->second, out_variable);
            }
        }

        return success;
    }

    [[nodiscard]] 
    bool ParseJsonFromStream(std::istream& in_stream);

private:
    JsonObject root_object_;

    std::stack<JsonObject*> context_stack_;
};

} // namespace fade

#endif // FADE_CORE_SERIALIZATION_JSON_INPUT_ARCHIVE_HPP_
