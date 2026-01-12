#ifndef FADE_APPLICATION_HPP_
#define FADE_APPLICATION_HPP_

#define FADE_PLATFORM_UNIX 1

#include "application.generated.hpp"

#include "application/include/command_line_arguments.hpp"

#include <string>
#include <vector>
#include <memory>

namespace fade::application {

class FADE_APPLICATION_API ApplicationBase
{
public:
    /** Entrypoint of the application */
    virtual void Entry(const CommandLineArguments& in_args) = 0;

    virtual const std::string_view GetApplicationName() const = 0;
};


/** 
* Getter function for the implementation
* 
* This could be a singleton, if an application can only have one instance.
* Or it can behave as a factory, if the application supports it and the user desires it.
*/
std::unique_ptr<ApplicationBase> FADE_APPLICATION_API Create();

}

#endif // FADE_APPLICATION_HPP_