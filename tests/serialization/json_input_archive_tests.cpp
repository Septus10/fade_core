#include "core/include/serialization/json_input_archive.hpp"
#include "core/include/serialization/serialization.hpp"

#include "gtest/gtest.h"

#include <sstream>

struct ObjectTest 
{
    std::string a;

    int b;

    bool c;

    double d;
};

template <fade::InputArchiveType ArchiveType>
bool Serialize(ArchiveType& in_archive, ObjectTest& out_object_test)
{
    ARCHIVE_PARAM(in_archive, out_object_test, a);
    ARCHIVE_PARAM(in_archive, out_object_test, b);
    ARCHIVE_PARAM(in_archive, out_object_test, c);

    return true;
}

const std::string input_json = "{\"a\": \"hello world!\", \"b\": 10, \"c\": true, \"d\": 5.7}";

TEST(core_serialization_json, deserialize_into_object) 
{
    fade::JsonInputArchive json_input_archive;
    
    // First we have the input archive parse
    ASSERT_TRUE(json_input_archive.Parse(input_json));

    // Now we try to deserialize
    ObjectTest obj_test;
    ASSERT_TRUE(Serialize(json_input_archive, obj_test));

    ASSERT_EQ(obj_test.a, "hello world!");
    ASSERT_EQ(obj_test.b, 10);
    ASSERT_EQ(obj_test.c, true);
    ASSERT_DOUBLE_EQ(obj_test.d, 5.7);
}