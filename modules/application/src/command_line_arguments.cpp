#include "application/include/command_line_arguments.hpp"

// Fade includes
#include "core/include/logging.hpp"

// STL includes
#include <sstream>
#include <print>
#include <iomanip>

namespace fade::application {

CommandLineArgumentDescription::CommandLineArgumentDescription(std::string in_short_name, std::string in_long_name, std::string in_description)
    : short_name(in_short_name)
    , long_name(in_long_name)
    , description(in_description)
{ 
    CommandLineArguments::RegisterCommandLineArgument(*this);
}

bool operator==(const CommandLineArgumentDescription& in_lhs, const CommandLineArgumentDescription& in_rhs) noexcept
{
    return in_lhs.short_name == in_rhs.short_name || in_lhs.long_name == in_rhs.long_name;
}

bool operator==(const CommandLineArgumentDescription& in_lhs, const std::string& in_rhs) noexcept
{
    return in_lhs.short_name == in_rhs || in_lhs.long_name == in_rhs;
}

CommandLineArguments::~CommandLineArguments()
{
    arguments_.clear();
    GetRegisteredDescriptions().clear();
}

const CommandLineArgument* CommandLineArguments::Get(const CommandLineArgumentDescription& in_description) const
{
    return Get(in_description.short_name.c_str(), in_description.long_name.c_str());
}

const CommandLineArgument* CommandLineArguments::Get(const char* in_short_name_, const char* in_long_name_ /*= ""*/) const
{
    for (auto& arg : arguments_)
    {
        if (arg.description != nullptr && (arg.description->short_name == in_short_name_ || (!in_long_name_ || arg.description->long_name == in_long_name_)))
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
        std::string found_name;
        if (arg_str.size() >= 2 && arg_str[1] == '-')
        {
            // Long name
            found_name = arg_str.substr(2);
        }
        else
        {
            // Short name
            found_name = arg_str.substr(1);
        }

        // Try and find the corresponding description
        for (const std::vector<CommandLineArgumentDescription>& registered_argument_descriptions = GetRegisteredDescriptions(); const CommandLineArgumentDescription& description : registered_argument_descriptions)
        {
            if (description == found_name)
            {
                arg.description = &description;
                break;
            }
        }

        // Collect values
        while (i + 1 < in_argc)
        {
            std::string next_arg = in_argv[i + 1];
            if (next_arg.size() > 0 && next_arg[0] == '-')
            {
                break; // Next argument is another flag
            }

            // Only if we have found a valid description will we add values to this argument
            if (arg.description != nullptr)
            {
                arg.values.push_back(next_arg);
            }
            ++i;
        }

        if (arg.description != nullptr)
        {
            arguments_.push_back(std::move(arg));
        }
    }
}

void CommandLineArguments::PrintHelpString(const std::string_view in_application_name) const
{
    std::print("Listing command line arguments for {}\n", in_application_name);
    std::stringstream help_string_stream;

    for (const std::vector<CommandLineArgumentDescription>& registered_argument_descriptions = GetRegisteredDescriptions(); const CommandLineArgumentDescription& description : registered_argument_descriptions)
    {
        help_string_stream << std::format("{:<20}{:<40}{:<40}\n", "-" + description.short_name, "--" + description.long_name, description.description);
    }

    std::cout << help_string_stream.str();
}

void CommandLineArguments::RegisterCommandLineArgument(const CommandLineArgumentDescription& in_command_line_argument_description)
{
    // Check if we already have overlap in either the short or long name
    std::vector<CommandLineArgumentDescription>& registered_argument_descriptions = GetRegisteredDescriptions();
    for (const CommandLineArgumentDescription& description : registered_argument_descriptions)
    {
        if (description == in_command_line_argument_description)
        {
            fade::Log<fade::LogLevel::kWarning>("Unable to add command line argument ({} - {}) as there appears to be overlap with another command line argument ({} - {})"
                , in_command_line_argument_description.short_name, in_command_line_argument_description.long_name
                , description.short_name, description.long_name);
            
            return;
        }
    }

    registered_argument_descriptions.push_back(in_command_line_argument_description);
}

std::vector<CommandLineArgumentDescription>& CommandLineArguments::GetRegisteredDescriptions()
{
    static std::vector<CommandLineArgumentDescription> registered_descriptions;
    return registered_descriptions;
}

}