#include <iostream>
#include <vector>
#include <utility>
#include <print>

#include "application/interface/application.hpp"
#include "core/include/type_definitions.hpp"
#include "main/interface/fade_main.hpp"

int main(int in_argc, char** in_args)
{
    return fade::main::FadeMain(in_argc, in_args);
}