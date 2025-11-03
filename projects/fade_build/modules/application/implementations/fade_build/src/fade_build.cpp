#include "application/interface/application.hpp"

#include "fade_build.hpp"

#include <iostream>
#include <sstream>
#include <print>
#include <filesystem>
#include <map>
#include <any>
#include <type_traits>
#include <filesystem>
#include <fstream>

namespace fade::serialization {

template <typename T>
using name_value_pair = std::pair<const std::string, T*>;

template <typename T>
using const_name_value_pair = std::pair<const std::string&, const T*>;

class InputArchive
{
public:
    virtual void Load(const std::string& in_name, bool& out_bool) = 0;
    virtual void Load(const std::string& in_name, std::string& out_string) = 0;

    virtual void SetContext(const std::string& in_context) = 0;
};


class OutputArchive
{

};

template <typename T>
concept ArchiveClass = std::derived_from<T, OutputArchive> || std::derived_from<T, InputArchive>;

template <ArchiveClass ArchiveType, class ObjType>
bool Serialize(ArchiveType& in_output_archive, ObjType& in_obj);

template <typename ArchiveType, typename T>
concept SerializableObject = requires(ArchiveType& archive, T& obj)
{
    { Serialize(archive, obj) };
};

template <ArchiveClass ArchiveType, typename T>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<T> in_obj) requires SerializableObject<ArchiveType, T>
{
    in_archive.SetContext(in_obj.first);
    Serialize(in_archive, *in_obj.second);
    return in_archive;
}

template <ArchiveClass ArchiveType>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<bool> in_name_bool_pair)
{
    // Implementation for bool type
    return in_archive;
}

template <ArchiveClass ArchiveType>
ArchiveType& operator<<(ArchiveType& in_archive, name_value_pair<std::string> in_name_string_pair)
{
    // Implementation for string type
    in_archive.Load(in_name_string_pair.first, *in_name_string_pair.second);
    return in_archive;
}

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

struct JsonValue
{
    JsonValueType type;

    virtual ~JsonValue() = default;
};

struct JsonObject
{
    ~JsonObject()
    {
        for (auto& member : members)
        {
            delete member.second;
        }
    }

    std::map<std::string, JsonValue*> members;
};

struct JsonStringValue : public JsonValue
{
    std::string string_value;
};

struct JsonNumberValue : public JsonValue
{
    union {
        int64_t int_value;
        double floating_point_value;
    } number_value;
};

struct JsonObjectValue : public JsonValue
{
    JsonObject object_value;
    JsonObject* parent_object_ = nullptr;
};

struct JsonArrayValue : public JsonValue
{
    std::vector<JsonValue*> array_values;
};

struct JsonBoolValue : public JsonValue
{
    bool bool_value;
};

struct JsonNullValue : public JsonValue
{
    // No additional data needed for null
};

class JsonInputArchive : public InputArchive 
{
public:
    // No default constructor, we need to provide a file stream or string to read from
    JsonInputArchive() = delete;
    JsonInputArchive(std::ifstream& in_file_stream)
    {
        // Move read position to end of the file to read the size
        in_file_stream.seekg(0, std::ios::end);
        size_t size = in_file_stream.tellg();

        // allocate json string of the file size
        std::string json_string(size, ' ');

        // Move read position to beginning and read the file content into the string
        in_file_stream.seekg(0);
        in_file_stream.read(&json_string[0], size);

        // Finally parse the string
        ParseJsonString(json_string);
    }

    JsonInputArchive(std::string& in_json_string)
    {
        ParseJsonString(in_json_string);
    }

    virtual void Load(const std::string& in_name, bool& out_bool) override
    {
        LoadInternal<bool, JsonBoolValue>(in_name, out_bool);
    }

    virtual void Load(const std::string& in_name, std::string& out_string) override
    {
        LoadInternal<std::string, JsonStringValue>(in_name, out_string);
    }

    virtual void SetContext(const std::string& in_context) override
    {
        if (context_object_ != nullptr && context_object_->members.find(in_context) != context_object_->members.end())
        {
            if (auto* json_object_value = dynamic_cast<JsonObjectValue*>(context_object_->members[in_context]); json_object_value != nullptr)
            {
                context_object_ = &json_object_value->object_value;
            }
            else
            {
                std::print("[\033[31mError\033[0m]: Context '{}' is not a JSON object.\n", in_context);
            }
        }
        else
        {
            std::print("[\033[33mWarning\033[0m]: Context '{}' not found in current JSON object.\n", in_context);
        }
    }

private:
    template <typename T, typename JsonValueType>
    T GetValue(JsonValueType* in_json_value)
    {
        if constexpr (std::is_same_v<T, bool> && std::is_same_v<JsonValueType, JsonBoolValue>)
        {
            return in_json_value->bool_value;
        }
        else if constexpr (std::is_same_v<T, std::string> && std::is_same_v<JsonValueType, JsonStringValue>)
        {
            return in_json_value->string_value;
        }
        else if constexpr (std::is_same_v<T, std::string> && std::is_same_v<JsonValueType, JsonNullValue>)
        {
            return "null";
        }
        else
        {
            static_assert(false, "Unsupported type for GetValue");
        }
    }

    template <typename T, typename JsonValueType>
    void LoadInternal(const std::string& in_name, T& out_value)
    {
        if (context_object_ != nullptr)
        {
            auto it = context_object_->members.find(in_name);
            if (it != context_object_->members.end())
            {
                if (auto* json_value = dynamic_cast<JsonValueType*>(it->second); json_value != nullptr)
                {
                    out_value = GetValue<T, JsonValueType>(json_value);
                    return;
                }
                else
                {
                    std::print("[\033[31mError\033[0m]: Failed to cast JSON value for key '{}' to expected type.\n", in_name);
                }
            }
            else
            {
                std::print("[\033[33mWarning\033[0m]: Key '{}' not found in JSON data.\n", in_name);
            }
        }
    }


    void ParseJsonString(std::string& in_json_string)
    {
        if (in_json_string.empty())
        {
            std::print("[\033[31mError\033[0m]: Cannot parse empty JSON string.\n");
            return;
        }

        size_t pos = 0;
        if (in_json_string[pos] == '{')
        {
            size_t obj_start = pos;
            size_t brace_count = 1;
            pos++;
            while (pos < in_json_string.size() && brace_count > 0)
            {
                if (in_json_string[pos] == '{')
                    brace_count++;
                else if (in_json_string[pos] == '}')
                    brace_count--;
                pos++;
            }

            if (brace_count == 0)
            {
                std::string obj_string = in_json_string.substr(obj_start + 1, pos - obj_start - 2);
                ParseJsonObject(obj_string, root_object_);
            }
            else
            {
                std::print("[\033[31mError\033[0m]: Mismatched braces in JSON string.\n");
            }
        }
        else
        {
            std::print("[\033[31mError\033[0m]: JSON string must start with an object.\n");
        }
    }

    void ParseJsonObject(std::string& in_json_object_string, JsonObject& out_json_object)
    {
        for (std::size_t pos = 0; pos < in_json_object_string.size(); ++pos)
        {
            const char& character = in_json_object_string[pos];
            if (std::isspace(character))
            {
                continue;
            }

            // Skip comments
            if (character == '/' && pos + 1 < in_json_object_string.size() && in_json_object_string[pos + 1] == '*')
            {
                pos = in_json_object_string.find("*/", pos + 3);
                continue;
            }

            // If we find the start of a string, let's try and find the key and parse the value
            if (character == '"')
            {
                std::size_t key_start = pos + 1;
                std::size_t key_end = in_json_object_string.find('"', key_start);
                if (key_end == std::string::npos)
                {
                    std::print("[\033[31mError\033[0m]: Unterminated string in JSON object.\n");
                    return;
                }
                std::string key = in_json_object_string.substr(key_start, key_end - key_start);

                // Skip whitespace and check the colon
                std::size_t colon_pos = in_json_object_string.find(':', key_end);
                if (colon_pos == std::string::npos)
                {
                    std::print("[\033[31mError\033[0m]: Missing ':' after key '{}' in JSON object.\n", key);
                    return;
                }

                std::size_t value_start = colon_pos + 1;
                pos = value_start + ParseJsonValue(in_json_object_string.substr(value_start), out_json_object.members[key]);
            }
        }
    }

    std::size_t ParseJsonValue(std::string_view in_json_value_string, JsonValue*& out_json_value)
    {
        for (std::size_t pos = 0; pos < in_json_value_string.size(); ++pos)
        {
            const char& character = in_json_value_string[pos];
            if (std::isspace(character))
            {
                continue;
            }

            if (character == ',' || character == '}')
            {
                return pos + 1; // End of value
            }

            // Handle different JSON value types here (string, number, object, array, true, false, null)
            if (character == '"')
            {
                pos = ParseJsonStringValue(in_json_value_string.substr(pos + 1), out_json_value);
            }
            else if (std::isdigit(character) || character == '-')
            {
                pos = ParseJsonNumberValue(in_json_value_string.substr(pos + 1), out_json_value);
            }
            else if (character == '{')
            {
                pos = ParseJsonObjectValue(in_json_value_string.substr(pos + 1), out_json_value);
            }
            else if (character == '[')
            {
                pos = ParseJsonArrayValue(in_json_value_string.substr(pos + 1), out_json_value);
            }
            else if (in_json_value_string.compare(pos, 4, "true") == 0)
            {
                out_json_value = new JsonBoolValue();
                static_cast<JsonBoolValue*>(out_json_value)->bool_value = true;
                out_json_value->type = JsonValueType::FALSE;
                pos += 4; // +4 because the loop will increment pos by 1
            }
            else if (in_json_value_string.compare(pos, 5, "false") == 0)
            {
                out_json_value = new JsonBoolValue();
                static_cast<JsonBoolValue*>(out_json_value)->bool_value = false;
                out_json_value->type = JsonValueType::FALSE;
                pos += 4; // +4 because the loop will increment pos by 1
            }
            else if (in_json_value_string.compare(pos, 4, "null") == 0)
            {
                out_json_value = new JsonNullValue();
                out_json_value->type = JsonValueType::NULL;
                pos += 3; // +3 because the loop will increment pos by 1
            }
        }

        return 0;
    }

    std::size_t ParseJsonStringValue(std::string_view in_json_value_string, JsonValue*& out_json_value)
    {
        // Parse string value
        std::size_t value_end = in_json_value_string.find('"', 0);
        if (value_end == std::string::npos)
        {
            std::print("[\033[31mError\033[0m]: Unterminated string in JSON value.\n");
            return 0;
        }

        JsonStringValue* string_value = new JsonStringValue();
        string_value->string_value = in_json_value_string.substr(0, value_end);
        string_value->type = JsonValueType::STRING;
        out_json_value = string_value;
        return value_end + 2; // +2 to account for the closing quote
    }

    std::size_t ParseJsonNumberValue(std::string_view in_json_value_string, JsonValue*& out_json_value)
    {
        // Parse number value
        size_t pos = 0;
        while (pos < in_json_value_string.size() && (std::isdigit(in_json_value_string[pos]) || in_json_value_string[pos] == '.' || in_json_value_string[pos] == '-'))
        {
            pos++;
        }

        std::string number_str = std::string(in_json_value_string.substr(0, pos));
        JsonNumberValue* number_value = new JsonNumberValue();
        if (number_str.find('.') != std::string::npos)
        {
            number_value->number_value.floating_point_value = std::stod(number_str);
        }
        else
        {
            number_value->number_value.int_value = std::stoll(number_str);
        }
        number_value->type = JsonValueType::NUMBER;
        out_json_value = number_value;
        return pos;
    }

    std::size_t ParseJsonObjectValue(std::string_view in_json_value_string, JsonValue*& out_json_value)
    {
        // Parse object value
        size_t brace_count = 1;
        size_t pos = 0;
        while (pos < in_json_value_string.size() && brace_count > 0)
        {
            if (in_json_value_string[pos] == '{')
                brace_count++;
            else if (in_json_value_string[pos] == '}')
                brace_count--;

            if (brace_count == 0)
            {
                break;
            }

            pos++;
        }

        if (brace_count == 0)
        {
            std::string obj_string = std::string(in_json_value_string.substr(0, pos));
            JsonObjectValue* object_value = new JsonObjectValue();
            object_value->parent_object_ = context_object_;
            object_value->type = JsonValueType::OBJECT;
            ParseJsonObject(obj_string, object_value->object_value);
            out_json_value = object_value;
            return pos;
        }
        else
        {
            std::print("[\033[31mError\033[0m]: Mismatched braces in JSON object value.\n");
            return 0;
        }
    }

    std::size_t ParseJsonArrayValue(std::string_view in_json_value_string, JsonValue*& out_json_value)
    {
        // Parse array value
        JsonArrayValue* array_value = new JsonArrayValue();
        array_value->type = JsonValueType::ARRAY;

        size_t pos = 0;
        while (pos < in_json_value_string.size())
        {
            const char& character = in_json_value_string[pos];
            if (std::isspace(character))
            {
                pos++;
                continue;
            }

            if (character == ']')
            {
                pos++;
                break; // End of array
            }

            JsonValue* element_value = nullptr;
            ParseJsonValue(in_json_value_string.substr(pos), element_value);
            if (element_value != nullptr)
            {
                array_value->array_values.push_back(element_value);
            }

            // Skip comma if present
            if (in_json_value_string[pos] == ',')
            {
                pos++;
            }
        }

        out_json_value = array_value;
        return pos;
    }

private:
    JsonObject root_object_;
    JsonObject* context_object_ = &root_object_;
};

class JsonOutputArchive : public OutputArchive
{

};

}

struct ProjectApplicationModule
{
    std::string implementation;
    std::string parent_implementation;
};

// Used, for the time being, to prepare for reflection. 
// Since we would no longer require to manually write the name of the field with reflection.
#define ARCHIVE_PARAM(archive, obj, field) \
if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, OutputArchive>) {\
    in_archive << fade::serialization::const_name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
} else if constexpr (std::derived_from<std::remove_reference_t<decltype(archive)>, InputArchive>) { \
    in_archive << fade::serialization::name_value_pair<std::remove_reference_t<decltype(obj.field)>>(#field, &obj.field); \
}

namespace fade::serialization {

template<ArchiveClass ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectApplicationModule& in_project_application_module)
{
    ARCHIVE_PARAM(in_archive, in_project_application_module, implementation)
    ARCHIVE_PARAM(in_archive, in_project_application_module, parent_implementation)
    return true;
}

}

struct ProjectConfiguration
{
    std::string name;
    std::string description;
    std::string version;

    std::string entry_module_implementation;

    ProjectApplicationModule application_module;

    bool operator==(const ProjectConfiguration& other) const
    {
        return name == other.name && version == other.version;
    }
};

namespace fade::serialization {

template <ArchiveClass ArchiveType>
bool Serialize(ArchiveType& in_archive, ProjectConfiguration& in_project_configuration)
{
    ARCHIVE_PARAM(in_archive, in_project_configuration, name)
    ARCHIVE_PARAM(in_archive, in_project_configuration, description)
    ARCHIVE_PARAM(in_archive, in_project_configuration, version)
    ARCHIVE_PARAM(in_archive, in_project_configuration, entry_module_implementation)
    ARCHIVE_PARAM(in_archive, in_project_configuration, application_module)

    return true;
}

}

void PrintHelpString()
{
    std::stringstream help_stream;
    help_stream << "Test application usage:\n";
    help_stream << "\t-h\t\tPrints this help string\n"; 
    std::print("{}", help_stream.str());
}

void FadeBuild::Entry(const fade::application::CommandLineArguments& in_args)
{
    if (in_args.Get("h", "help") != nullptr)
    {
        PrintHelpString();
        return;
    }

    const auto GetProjectFilePath = [](const fade::application::CommandLineArguments& in_args, std::filesystem::path& out_path) -> bool
    {
        if (const fade::application::CommandLineArgument* project_arg = in_args.Get("p", "project"); project_arg != nullptr)
        {
            if (project_arg->values.size() < 1)
            {
                std::print("[\033[31mError\033[0m]: No project file specified.\n");
                return false;
            }

            if (project_arg->values.size() > 1)
            {
                std::print("[\033[33mWarning\033[0m]: Multiple project files specified. Only the first one will be used.\n");
            }

            out_path = project_arg->values[0];
            // check for project path validity
            if (out_path.string().contains(".fproject") == false)
            {
                std::print("[\033[31mError\033[0m]: Invalid project file specified: {}. Must have .fproject extension.\n", out_path.string());
                return false;
            }
        }

        return true;
    };

    if (std::filesystem::path project_file_path; GetProjectFilePath(in_args, project_file_path))
    {
        if (project_file_path.empty())
        {
            std::print("[\033[31mError\033[0m]: Specified project file path is empty.\n");
            return;
        }

        if (project_file_path.is_relative())
        {
            project_file_path = std::filesystem::absolute(project_file_path).lexically_normal();
        }

        if (!std::filesystem::exists(project_file_path))
        {
            std::print("[\033[31mError\033[0m]: Specified project file path does not exist: {}.\n", project_file_path.string());
            return;
        }
        
        std::print("Using project file: {}\n", project_file_path.string());

        // Load project configuration
        ProjectConfiguration project_config;
        std::ifstream project_file_stream(project_file_path, std::ios::in);
        fade::serialization::JsonInputArchive json_input_archive(project_file_stream);
        if (Serialize(json_input_archive, project_config))
        {
            std::print("Project Name: {}\n", project_config.name);
            std::print("Project Description: {}\n", project_config.description);
            std::print("Project Version: {}\n", project_config.version);
            std::print("Entry Module Implementation: {}\n", project_config.entry_module_implementation);
            std::print("Application Module Implementation: {}\n", project_config.application_module.implementation);
            std::print("Application Module Parent Implementation: {}\n", project_config.application_module.parent_implementation);
        }
    }
}

namespace fade::application {

std::unique_ptr<ApplicationBase> Create()
{
    return std::make_unique<FadeBuild>();
}

}