#pragma once

#include <string>
#include <type_traits>
#include <vector>
#include <map>
#include <cstdint>

namespace fade::serialization {

// Name-value pair helper used with operator<<
template<typename T>
struct Nvp {
    const char* name;
    T& value;
};

template<typename T>
inline Nvp<T> nvp(const char* name, T& value) {
    return Nvp<T>{name, value};
}

// Forward declaration for ADL-based custom serializers
template<typename Archive, typename T>
void serialize(Archive& ar, T& v);

// Base class for input archives. It implements the generic logic for
// reading named fields and delegates format-specific lookup operations
// to virtual protected methods implemented by concrete archives.
class input_archive {
public:
    input_archive() = default;
    virtual ~input_archive() = default;

    // Primary interface used by callers: archive << nvp("field", var);
    template<typename T>
    input_archive& operator<<(Nvp<T> n) {
        load(n.name, n.value);
        return *this;
    }

protected:
    // format-specific queries implemented by concrete archives
    // They operate against the current "scope" (object or array element)
    virtual bool has_member(const std::string& name) const = 0;
    virtual bool is_null(const std::string& name) const = 0;
    virtual bool read_bool(const std::string& name, bool& out) const = 0;
    virtual bool read_number(const std::string& name, double& out) const = 0;
    virtual bool read_string(const std::string& name, std::string& out) const = 0;

    // Enter/exit nested object/array context. For example when deserializing
    // a user-defined struct the archive implementation should make calls to
    // push the object scope so subsequent read_* calls lookup inside it.
    // The default implementations assume a tree-like navigator provided by
    // the concrete archive.
    virtual bool enter_object(const std::string& name) = 0; // returns true on success
    virtual void exit_object() = 0;

    virtual bool enter_array(const std::string& name, size_t& out_count) = 0;
    virtual void exit_array() = 0;

private:
    // Generic loader that dispatches based on T
    template<typename T>
    std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T,bool>>
    load_impl(const std::string& name, T& out) {
        double tmp;
        if (read_number(name, tmp)) {
            out = static_cast<T>(tmp);
        } else {
            // attempt to read as string and convert
            std::string s;
            if (read_string(name, s)) {
                out = static_cast<T>(std::stoll(s));
            } else {
                // leave out unchanged if not found; implementations may prefer to error
            }
        }
    }

    template<typename T>
    std::enable_if_t<std::is_floating_point_v<T>>
    load_impl(const std::string& name, T& out) {
        double tmp;
        if (read_number(name, tmp)) {
            out = static_cast<T>(tmp);
        } else {
            std::string s;
            if (read_string(name, s)) {
                out = static_cast<T>(std::stod(s));
            }
        }
    }

    void load_impl(const std::string& name, bool& out) {
        bool b;
        if (read_bool(name, b)) {
            out = b;
        } else {
            std::string s;
            if (read_string(name, s)) {
                out = (s == "true" || s == "1");
            }
        }
    }

    void load_impl(const std::string& name, std::string& out) {
        read_string(name, out);
    }

    template<typename Elem>
    void load_impl(const std::string& name, std::vector<Elem>& out) {
        size_t n = 0;
        if (!enter_array(name, n)) return;
        out.clear(); out.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            // enter array element by index - concrete archives should provide
            // semantics where entering the array will point indexing reads at
            // the i-th element.
            // We simulate this by asking the archive to enter an element named
            // by the numeric index string. Concrete JSON archive below supports this.
            std::string idx = std::to_string(i);
            Elem e{};
            load(idx, e);
            out.push_back(std::move(e));
        }
        exit_array();
    }

    // Fallback for user-defined types: call ADL serialize(Archive&, T&)
    template<typename T>
    std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T,std::string> && !std::is_same_v<T,bool> && !std::is_same_v<T,std::vector<typename T::value_type>>, void>
    load_impl(const std::string& name, T& out) {
        if (!enter_object(name)) return;
        // call user-provided serialize overload
        serialize(*this, out);
        exit_object();
    }

    // Top-level loader: locates the child node and forwards to load_impl
    template<typename T>
    void load(const std::string& name, T& out) {
        if (!has_member(name)) {
            // missing member: keep default or allow implementations to alter behaviour
            return;
        }
        if (is_null(name)) {
            // leave default value
            return;
        }
        load_impl(name, out);
    }
};

} // namespace fade::serialization
