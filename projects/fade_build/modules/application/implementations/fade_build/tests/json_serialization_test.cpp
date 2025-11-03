#include "fade_build/src/fade_build.hpp"

#include "gtest/gtest.h"

struct serialization_nested_test_struct
{
    int nested_int;
    std::string nested_string;

    operator==(const serialization_nested_test_struct& other) const
    {
        return nested_int == other.nested_int &&
               nested_string == other.nested_string;
    }
};

struct serialization_test_struct
{
    bool test_bool;
    float test_float;
    serialization_nested_test_struct nested_struct;

    operator==(const serialization_test_struct& other) const
    {
        return test_bool == other.test_bool &&
               test_float == other.test_float &&
               nested_struct == other.nested_struct;
    }
}

serialization_test_struct test_obj = {true, 3.14f, {42, "Hello World!"}};

std::string test_json = R"(
{
    "test_bool": true,
    "test_float": 3.14,
    "nested_struct": {
        "nested_int": 42,
        "nested_string": "Hello World!"
    }
}
    )";

TEST(JsonSerializationTest, Serialize)
{
    fade_build::json_archive j = fade_build::json_serialize<serialization_test_struct>(test_obj);

    EXPECT_EQ(j.to_string(), json_string);
}

TEST(JsonSerializationTest, Deserialize)
{
    // Create json archive and load from string
    fade_build::json_archive j;
    j.from_string(test_json);

    // Deserialize the json archive to 
    serialization_test_struct deserialized_obj = fade_build::json_deserialize<serialization_test_struct>(j);

    EXPECT_EQ(deserialized_obj, test_obj);
}