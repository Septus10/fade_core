#include "main/interface/fade_main.hpp"

#include "application/interface/application.hpp"

#include <memory>
#include <string>
#include <vector>

namespace fade::main {

int FadeMain(int in_argc, char** in_args)
{
    std::unique_ptr<fade::application::ApplicationBase> app = fade::application::Create();
    if (app.get() != nullptr)
    {
        fade::application::CommandLineArguments cmd_args;
        cmd_args.Parse(in_argc, in_args);
        app->Entry(cmd_args);
    }

    return 0;
}

}