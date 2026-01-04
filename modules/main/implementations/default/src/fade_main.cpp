#include "main/interface/fade_main.hpp"

#include "application/interface/application.hpp"

#include <memory>
#include <string>
#include <vector>
//#include <stacktrace>
#include <print>

namespace fade::main {

int FadeMain(int in_argc, char** in_args)
{
    std::unique_ptr<fade::application::ApplicationBase> app = fade::application::Create();
    if (app.get() != nullptr)
    {
        fade::application::CommandLineArguments cmd_args;
        cmd_args.Parse(in_argc, in_args);
        try
        {
            app->Entry(cmd_args);
        }
        catch(const std::exception& e)
        {
            std::print("Unhandled exception caught {}\n", e.what());
            //std::print("{}\n", std::to_string(std::stacktrace::current()));
            return EXIT_FAILURE;
        }
    }

    return 0;
}

}