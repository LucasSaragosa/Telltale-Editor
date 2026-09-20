#pragma once

#include <TelltaleEditor.hpp>

class Application
{
public:

	static I32 RunApplication(const std::vector<CommandLine::TaskArgument>& args); // runs the application (from command line)

protected:

	I32 _Run(const std::vector<CommandLine::TaskArgument>& args);

};