#ifndef FADE_APPLICATION_COMMAND_LINE_ARGUMENTS_HPP_
#define FADE_APPLICATION_COMMAND_LINE_ARGUMENTS_HPP_

// Fade includes
#include "core/include/containers/dynamic_array.hpp"

// STL includes
#include <string>

namespace fade::application {

struct CommandLineArgumentDescription
{
    CommandLineArgumentDescription() = default;
    CommandLineArgumentDescription(std::string in_short_name, std::string in_long_name, std::string in_description);

    CommandLineArgumentDescription(const CommandLineArgumentDescription& in_other) = default;
    CommandLineArgumentDescription(CommandLineArgumentDescription&& in_other) = default;

    std::string short_name;
    std::string long_name;
    std::string description;
};

bool operator==(const CommandLineArgumentDescription& in_lhs, const CommandLineArgumentDescription& in_rhs) noexcept;
bool operator==(const CommandLineArgumentDescription& in_lhs, const std::string& in_rhs) noexcept;

struct CommandLineArgument
{
    const CommandLineArgumentDescription* description = nullptr;
    fade::DynamicArray<std::string> values;
};

class CommandLineArguments
{
public:
    ~CommandLineArguments();

    const CommandLineArgument* Get(const CommandLineArgumentDescription& in_description) const;
    const CommandLineArgument* Get(const char* in_shortname, const char* in_longname = "") const;

    void Parse(int in_argc, char** in_argv);

    void PrintHelpString(const std::string_view in_application_name) const;

    static void RegisterCommandLineArgument(const CommandLineArgumentDescription& in_command_line_argument_description);

private:
    static fade::DynamicArray<CommandLineArgumentDescription>& GetRegisteredDescriptions();

private:
    fade::DynamicArray<CommandLineArgument> arguments_;

    
};

}

#endif