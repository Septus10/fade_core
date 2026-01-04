#include "core/include/serialization/json_input_archive.hpp"
#include "core/include/logging.hpp"

#include <filesystem>
#include <istream>
#include <iterator>
#include <cassert>
#include <cstring>

namespace fade::core {

// json parser state machine
// state machine tracks state changes, and json parser state machine uses those state changes

enum class JsonParserStates : fade::uint8 
{
    kObject,
    kArray,
    kValue,
    kString,
    kNumber,
    kWhitespace
};

bool JsonObject::IsValid() const
{
    for (auto& it: members)
    {
        if (it.second.value.index() == std::variant_npos)
        {
            return false;
        }

        if (std::holds_alternative<JsonObject>(it.second.value))
        {
            if (!std::get<JsonObject>(it.second.value).IsValid())
            {
                return false;
            }
        }

        if (std::holds_alternative<JsonArray>(it.second.value))
        {
            const JsonArray& json_array = std::get<JsonArray>(it.second.value);
            for (const JsonValue& array_element : json_array.elements)
            {
                if (array_element.value.index() == std::variant_npos)
                {
                    return false;
                }

                if (std::holds_alternative<JsonObject>(array_element.value))
                {
                    if (!std::get<JsonObject>(array_element.value).IsValid())
                    {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

template <typename T>
class ParserBase
{
public:
    ParserBase(std::istream_iterator<T> in_stream_iterator)
        : parse_stream_iterator_(in_stream_iterator)
    { }

    const T& CurrentToken()
    {
        assert(parse_stream_iterator_ != std::istream_iterator<T>());
        return *parse_stream_iterator_;
    }

    template <fade::size_t N>
    bool Consume(const T (&tokens_to_consume)[N])
    {
        for (fade::size_t i = 0; i < N - 1; ++i)
        {
            if (!Consume(tokens_to_consume[i]))
            {
                return false;
            }
        }

        return true;
    }
    
    bool Consume(const T& token_to_consume)
    {
        if (CurrentToken() == token_to_consume)
        {
            ++parse_stream_iterator_;
            return true;
        }

        return false;
    }


private:
    std::istream_iterator<T> parse_stream_iterator_;
};

class JsonParser : public ParserBase<char>
{
public:
    JsonParser(std::istream_iterator<char>&& in_stream_iterator)
        : ParserBase(in_stream_iterator)
    { }

    JsonObject Parse()
    {
        JsonObject return_object;
        if (!ParseJsonObject(return_object))
        {
            fade::core::Log<fade::core::LogLevel::kError>("Error whilst parsing json object: {}", error_message_);
        }

        if (!return_object.IsValid())
        {
            fade::core::Log<fade::core::LogLevel::kError>("Parsed JSON object is not valid.");
        }
        
        return return_object;
    }

    bool ParseJsonObject(JsonObject& out_json_object, int depth = 0)
    {
        const int max_depth = 100; // Prevent stack overflow
        if (depth > max_depth) {
            SetErrorMessage("JSON nesting depth exceeds maximum of {}", max_depth);
            return false;
        }

        if (!Consume('{'))
        {
            SetErrorMessage("Expected '{' but found {}", CurrentToken());
            return false;
        }

        ParseWhitespace();

        while (!Consume('}'))
        {
            ParseWhitespace();

            std::string value_name;
            if (!ParseString(value_name))
            {
                return false;
            }

            ParseWhitespace();

            if (!Consume(':'))
            {
                SetErrorMessage("Expected ':' but found {}", CurrentToken());
                return false;
            }

            JsonValue value;
            if (!ParseValue(value, depth + 1))
            {
                return false;
            }

            if (out_json_object.members.contains(value_name))
            {
                SetErrorMessage("Duplicate key '{}' found in JSON object.", value_name);
                return false;
            }

            out_json_object.members[value_name] = std::move(value);

            Consume(',');
        }

        return true;
    }

    bool ParseJsonArray(JsonArray& out_json_array, int depth = 0)
    {
        const int max_depth = 100;
        if (depth > max_depth) {
            SetErrorMessage("JSON nesting depth exceeds maximum of {}", max_depth);
            return false;
        }

        out_json_array.elements = std::vector<JsonValue>();
        if (!Consume('['))
        {
            SetErrorMessage("Expected '[' but found {}", CurrentToken());
            return false;
        }

        ParseWhitespace();

        while (!Consume(']'))
        {
            ParseWhitespace();

            JsonValue value;
            if (!ParseValue(value, depth + 1))
            {
                return false;
            }

            out_json_array.elements.push_back(std::move(value));

            Consume(',');
        }

        return true;
    }

    void ParseWhitespace()
    {
        char current_token = CurrentToken();
        while (std::isspace(current_token))
        {
            Consume(current_token);
            current_token = CurrentToken();
        }
    }

    bool ParseString(std::string& out_string)
    {
        if (!Consume('"'))
        {
            SetErrorMessage("Expected '\"' but found {}", CurrentToken());
            return false;
        }

        while (!Consume('"'))
        {
            out_string += CurrentToken();
            Consume(CurrentToken());
        }

        return true;
    }

    bool ParseValue(JsonValue& out_value, int depth = 0)
    {
        ParseWhitespace();

        const char& current_token = CurrentToken();
        if (current_token == '"')
        {
            std::string string_value;
            if (!ParseString(string_value))
            {
                return false;
            }
            out_value.value = string_value;
        }
        else if (std::isdigit(current_token) || current_token == '-')
        {
            std::string number_str;
            char tok = current_token;
            while (std::isdigit(tok) || tok == '.' || tok == '-')
            {
                number_str += tok;
                Consume(tok);
                tok = CurrentToken();
            }
            out_value.value = std::stod(number_str);
        }
        else if (current_token == '{')
        {
            if (!ParseJsonObject(out_value.value.emplace<JsonObject>(), depth + 1))
            {
                return false;
            }
        }
        else if (current_token == '[')
        {
            if (!ParseJsonArray(out_value.value.emplace<JsonArray>(), depth + 1))
            {
                return false;
            }
        }
        else if (Consume("true"))
        {
            out_value.value = true;
        }
        else if (Consume("false"))
        {
            out_value.value = false;
        }
        else if (Consume("null"))
        {
            out_value.value = nullptr;
        }
        else
        {
            SetErrorMessage("Unexpected token while parsing value: {}", CurrentToken());
            return false;
        }

        ParseWhitespace();

        return true;
    }

private:
    template <typename... Args>
    void SetErrorMessage(std::string in_error_message_format, Args... in_args)
    {
        error_message_ = std::vformat(in_error_message_format, std::make_format_args(in_args...));;
    }

private:
    std::string error_message_;
};

bool JsonInputArchive::Parse(const std::string& in_json_string)
{
    std::istringstream string_stream(in_json_string);
    return ParseJsonFromStream(string_stream);
}

bool JsonInputArchive::Parse(std::ifstream& in_json_file_stream)
{
    return ParseJsonFromStream(in_json_file_stream);
}

bool JsonInputArchive::Parse(const std::filesystem::path& in_json_file_path)
{
    std::ifstream file_stream(in_json_file_path);
    if (!file_stream.is_open())
    {
        fade::core::Log<fade::core::LogLevel::kError>("Failed to open JSON file at path '{}'.", in_json_file_path.string());
        return false;
    }

    return ParseJsonFromStream(file_stream);
}

void JsonInputArchive::PushContext(JsonObject* in_json_object_ptr)
{
    context_stack_.push(in_json_object_ptr);
}

void JsonInputArchive::PopContext() 
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

bool JsonInputArchive::ParseJsonFromStream(std::istream& in_stream)
{
    // 
    if (!in_stream.good())
    {
        fade::core::Log<fade::core::LogLevel::kError>("Input stream is not in a good state for reading.");
        return false;
    }

    std::istream_iterator<char> iterator(in_stream >> std::noskipws);
    JsonParser json_parser(std::move(iterator));
    root_object_ = json_parser.Parse();
    context_stack_.push(&root_object_);
    
    return true;
}

}