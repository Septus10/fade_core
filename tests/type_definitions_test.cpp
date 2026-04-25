#include "core/include/type_definitions.hpp"

#include "gtest/gtest.h"

TEST(core, type_definition_sizes) {
    ASSERT_EQ(sizeof(fade::uint8), 1);
    ASSERT_EQ(sizeof(fade::uint16), 2);
    ASSERT_EQ(sizeof(fade::uint32), 4);
    ASSERT_EQ(sizeof(fade::uint64), 8);

    ASSERT_EQ(sizeof(fade::int8), 1);
    ASSERT_EQ(sizeof(fade::int16), 2);
    ASSERT_EQ(sizeof(fade::int32), 4);
    ASSERT_EQ(sizeof(fade::int64), 8);
}