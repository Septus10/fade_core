#ifndef FADE_CORE_TYPE_TRAITS_HPP_
#define FADE_CORE_TYPE_TRAITS_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"

// STL includes
#include <type_traits>
#include <memory>

namespace fade {

template <typename T>
concept UniquePtrType = std::is_same_v<T, std::unique_ptr<typename T::element_type, typename T::deleter_type>> 
    && requires { 
        typename T::element_type; 
        typename T::deleter_type; 
    };

template <typename T>
concept SharedPtrType = std::is_same_v<T, std::shared_ptr<typename T::element_type>>
    && requires {
        typename T::element_type;
    };

template <typename T>
concept VectorType = (std::derived_from<T, std::vector<typename T::value_type, typename T::allocator_type>> || std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>>)
    && requires {
        typename T::value_type;
        typename T::allocator_type;
    };

}

#endif // FADE_CORE_TYPE_TRAITS_HPP_