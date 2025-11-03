#ifndef FADE_BUILD_HPP
#define FADE_BUILD_HPP

#include "application/interface/application.hpp"

/**
 * Fade Build Application
 */
class FadeBuild : public fade::application::ApplicationBase
{
public:
    /**
     * Entry point of the application
     */
    virtual void Entry(const fade::application::CommandLineArguments& in_args) override;

private:
};

#endif // FADE_BUILD_HPP