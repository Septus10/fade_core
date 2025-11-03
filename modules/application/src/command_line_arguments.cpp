#include "application/include/command_line_arguments.hpp"

namespace fade::application {

const CommandLineArgument* CommandLineArguments::Get(const char* in_short_name_, const char* in_long_name_ /*= ""*/) const
{
    for (auto& arg : arguments_)
    {
        if (arg.short_name == in_short_name_ || (!in_long_name_ || arg.long_name == in_long_name_))
        {
            return &arg;
        }
    }

    return nullptr;
}

void CommandLineArguments::Parse(int in_argc, char** in_argv)
{
    for (int i = 1; i < in_argc; ++i)
    {
        std::string arg_str = in_argv[i];
        if (arg_str.size() < 1 || arg_str[0] != '-')
        {
            continue;
        }

        CommandLineArgument arg;
        if (arg_str.size() >= 2 && arg_str[1] == '-')
        {
            // Long name
            arg.long_name = arg_str.substr(2);
        }
        else
        {
            // Short name
            arg.short_name = arg_str.substr(1);
        }

        // Collect values
        while (i + 1 < in_argc)
        {
            std::string next_arg = in_argv[i + 1];
            if (next_arg.size() > 0 && next_arg[0] == '-')
            {
                break; // Next argument is another flag
            }
            arg.values.push_back(next_arg);
            ++i;
        }

        arguments_.push_back(std::move(arg));
    }
}

}