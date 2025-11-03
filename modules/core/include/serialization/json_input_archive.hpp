#pragma once

#include "input_archive.hpp"

#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace fade::serialization {

// Very small JSON value representation used by the example json input archive.
struct JsonValue {
    enum class Type { Null, Bool, Number, String, Object, Array };
    Type type = Type::Null;
    bool b = false;
    double num = 0.0;
    std::string str;
    std::map<std::string, JsonValue> obj;
    std::vector<JsonValue> arr;
};

// Minimal JSON parser. This is intentionally small and not feature-complete.
// It's good for examples and tests but not for production use. Replace with
// RapidJSON / nlohmann::json in a real project.
class JsonParser {
public:
    explicit JsonParser(std::string s) : src(std::move(s)), i(0) {}

    JsonValue parse() {
        skip_ws();
        JsonValue v = parse_value();
        skip_ws();
        return v;
    }

private:
    const std::string src;
    size_t i;

    void skip_ws() {
        while (i < src.size() && std::isspace((unsigned char)src[i])) ++i;
    }

    bool match(const char c) {
        skip_ws();
        if (i < src.size() && src[i] == c) { ++i; return true; }
        return false;
    }

    JsonValue parse_value() {
        skip_ws();
        if (i >= src.size()) throw std::runtime_error("unexpected end of input");
        char c = src[i];
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
        throw std::runtime_error(std::string("unexpected char: ") + c);
    }

    JsonValue parse_null() {
        expect("null");
        JsonValue v; v.type = JsonValue::Type::Null; return v;
    }

    JsonValue parse_bool() {
        if (peek_str("true")) { expect("true"); JsonValue v; v.type = JsonValue::Type::Bool; v.b = true; return v; }
        expect("false"); JsonValue v; v.type = JsonValue::Type::Bool; v.b = false; return v;
    }

    JsonValue parse_number() {
        size_t start = i;
        if (src[i] == '-') ++i;
        while (i < src.size() && std::isdigit((unsigned char)src[i])) ++i;
        if (i < src.size() && src[i] == '.') {
            ++i;
            while (i < src.size() && std::isdigit((unsigned char)src[i])) ++i;
        }
        // exponent not supported in this tiny parser
        double val = std::stod(src.substr(start, i - start));
        JsonValue v; v.type = JsonValue::Type::Number; v.num = val; return v;
    }

    JsonValue parse_string() {
        if (!match('"')) throw std::runtime_error("expected '" " to start string");
        std::string out;
        while (i < src.size()) {
            char c = src[i++];
            if (c == '\\') {
                if (i >= src.size()) break;
                char e = src[i++];
                if (e == '"') out.push_back('"');
                else if (e == '\\') out.push_back('\\');
                else if (e == '/') out.push_back('/');
                else if (e == 'b') out.push_back('\b');
                else if (e == 'f') out.push_back('\f');
                else if (e == 'n') out.push_back('\n');
                else if (e == 'r') out.push_back('\r');
                else if (e == 't') out.push_back('\t');
                else out.push_back(e);
            } else if (c == '"') {
                JsonValue v; v.type = JsonValue::Type::String; v.str = std::move(out); return v;
            } else {
                out.push_back(c);
            }
        }
        throw std::runtime_error("unterminated string");
    }

    JsonValue parse_object() {
        if (!match('{')) throw std::runtime_error("expected '{'");
        JsonValue v; v.type = JsonValue::Type::Object;
        skip_ws();
        if (match('}')) return v; // empty object
        while (true) {
            skip_ws();
            JsonValue key = parse_string();
            skip_ws();
            if (!match(':')) throw std::runtime_error("expected ':' in object");
            JsonValue val = parse_value();
            v.obj.emplace(std::move(key.str), std::move(val));
            skip_ws();
            if (match('}')) break;
            if (!match(',')) throw std::runtime_error("expected ',' in object");
        }
        return v;
    }

    JsonValue parse_array() {
        if (!match('[')) throw std::runtime_error("expected '['");
        JsonValue v; v.type = JsonValue::Type::Array;
        skip_ws();
        if (match(']')) return v; // empty
        while (true) {
            JsonValue el = parse_value();
            v.arr.push_back(std::move(el));
            skip_ws();
            if (match(']')) break;
            if (!match(',')) throw std::runtime_error("expected ',' in array");
        }
        return v;
    }

    bool peek_str(const char* s) {
        size_t len = std::strlen(s);
        return src.size() >= i + len && src.substr(i, len) == s;
    }

    void expect(const char* s) {
        size_t len = std::strlen(s);
        if (!peek_str(s)) throw std::runtime_error(std::string("expected: ") + s);
        i += len;
    }
};

// json_input_archive: concrete input archive that parses the whole JSON file
// into a JsonValue tree and provides lookup operations used by input_archive.
class json_input_archive : public input_archive {
public:
    // NOTE: assumption: user meant "open the file to read from" for an input
    // archive constructor. If you truly want to open for writing, let me know.
    explicit json_input_archive(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) throw std::runtime_error("failed to open file: " + path);
        std::ostringstream ss;
        ss << ifs.rdbuf();
        std::string content = ss.str();
        JsonParser p(std::move(content));
        root = p.parse();
        // stack initially points to the root (unnamed scope)
        scope_stack.clear();
        scope_stack.push_back(&root);
    }

    ~json_input_archive() override = default;

protected:
    bool has_member(const std::string& name) const override {
        const JsonValue* cur = current();
        if (!cur) return false;
        if (cur->type == JsonValue::Type::Object) {
            return cur->obj.find(name) != cur->obj.end();
        }
        if (cur->type == JsonValue::Type::Array) {
            // name may be an index
            size_t idx = parse_index(name);
            return idx < cur->arr.size();
        }
        // scalar: only empty name is valid
        return name.empty();
    }

    bool is_null(const std::string& name) const override {
        const JsonValue* node = lookup(name);
        return node && node->type == JsonValue::Type::Null;
    }

    bool read_bool(const std::string& name, bool& out) const override {
        const JsonValue* node = lookup(name);
        if (!node) return false;
        if (node->type == JsonValue::Type::Bool) { out = node->b; return true; }
        return false;
    }

    bool read_number(const std::string& name, double& out) const override {
        const JsonValue* node = lookup(name);
        if (!node) return false;
        if (node->type == JsonValue::Type::Number) { out = node->num; return true; }
        return false;
    }

    bool read_string(const std::string& name, std::string& out) const override {
        const JsonValue* node = lookup(name);
        if (!node) return false;
        if (node->type == JsonValue::Type::String) { out = node->str; return true; }
        return false;
    }

    bool enter_object(const std::string& name) override {
        const JsonValue* node = lookup(name);
        if (!node) return false;
        if (node->type != JsonValue::Type::Object) return false;
        scope_stack.push_back(node);
        return true;
    }

    void exit_object() override {
        if (!scope_stack.empty()) scope_stack.pop_back();
        if (scope_stack.empty()) scope_stack.push_back(&root);
    }

    bool enter_array(const std::string& name, size_t& out_count) override {
        const JsonValue* node = lookup(name);
        if (!node) return false;
        if (node->type != JsonValue::Type::Array) return false;
        out_count = node->arr.size();
        // push array node as scope so that subsequent numeric-index lookups work
        scope_stack.push_back(node);
        return true;
    }

    void exit_array() override {
        if (!scope_stack.empty()) scope_stack.pop_back();
        if (scope_stack.empty()) scope_stack.push_back(&root);
    }

private:
    JsonValue root;
    std::vector<const JsonValue*> scope_stack;

    const JsonValue* current() const {
        if (scope_stack.empty()) return &root;
        return scope_stack.back();
    }

    // lookup child by name inside the current scope. If name is empty, return current.
    const JsonValue* lookup(const std::string& name) const {
        const JsonValue* cur = current();
        if (!cur) return nullptr;
        if (name.empty()) return cur;
        if (cur->type == JsonValue::Type::Object) {
            auto it = cur->obj.find(name);
            if (it == cur->obj.end()) return nullptr;
            return &it->second;
        }
        if (cur->type == JsonValue::Type::Array) {
            size_t idx = parse_index(name);
            if (idx >= cur->arr.size()) return nullptr;
            return &cur->arr[idx];
        }
        // scalar can't have children
        return nullptr;
    }

    static size_t parse_index(const std::string& s) {
        if (s.empty()) return SIZE_MAX;
        size_t idx = 0;
        for (char c : s) {
            if (!std::isdigit((unsigned char)c)) return SIZE_MAX;
            idx = idx * 10 + (c - '0');
        }
        return idx;
    }
};

} // namespace fade::serialization
