#ifndef FADE_APPLICATION_COMMAND_LINE_ARGUMENTS_HPP_
#define FADE_APPLICATION_COMMAND_LINE_ARGUMENTS_HPP_

#include <string>
#include <vector>

namespace fade::application {

struct CommandLineArgument
{
    std::string short_name;
    std::string long_name;
    std::vector<std::string> values;
};

class CommandLineArguments
{
public:
    const CommandLineArgument* Get(const char* in_shortname, const char* in_longname = "") const;

    void Parse(int in_argc, char** in_argv);

private:
    std::vector<CommandLineArgument> arguments_;
};

}

#endif