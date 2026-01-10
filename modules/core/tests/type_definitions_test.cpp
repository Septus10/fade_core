#include <core/include/type_definitions.hpp>
#include <iostream>

#define TEST(test_namespace, test_suite, test_name) \ 
namespace test_namespace { \
struct test_##test_suite##_##test_name { \
void run_test(); \
};}\
void test_namespace::test_##test_suite##_##test_name::run_test()

#define ASSERT_EQ(x) if (!x) { std::cout << "failed." << std::endl; return; }

TEST(fade, type_definitions, type_sizes) 
{
    ASSERT_EQ(sizeof(fade::uint8) == 1)
    ASSERT_EQ(sizeof(fade::uint16) == 2)
    ASSERT_EQ(sizeof(fade::uint32) == 4)
    ASSERT_EQ(sizeof(fade::uint64) == 8)

    ASSERT_EQ(sizeof(fade::int8) == 1)
    ASSERT_EQ(sizeof(fade::int16) == 2)
    ASSERT_EQ(sizeof(fade::int32) == 4)
    ASSERT_EQ(sizeof(fade::int64) == 8)
}