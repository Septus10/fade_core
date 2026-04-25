#ifndef FADE_CORE_CONTAINERS_DYNAMIC_ARRAY_HPP_
#define FADE_CORE_CONTAINERS_DYNAMIC_ARRAY_HPP_

// Fade includes
#include "core/include/type_definitions.hpp"
#include "core/include/type_traits.hpp"

// STL includes
#include <vector>

namespace fade
{

/**
 * Fade Dynamic Array
 */
template <typename ElementType, typename Allocator = std::allocator<ElementType>>
class DynamicArray : public std::vector<ElementType, Allocator>
{
    using parent = std::vector<ElementType, Allocator>;

public:
    

public:
    /**
     * Remove At Swap
     * 
     * Swaps the element at the index and the last element, then pops the last element off the stack.
     * Prevents moving large areas of memory when removing elements.
     */
    void RemoveAtSwap(fade::size_t in_index)
    {
        parent::data()[in_index] = std::move(parent::data()[parent::size() - 1]);
        parent::pop_back();
    }
};

}

#endif // FADE_CORE_CONTAINERS_DYNAMIC_ARRAY_HPP_