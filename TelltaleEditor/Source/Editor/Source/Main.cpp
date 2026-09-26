#include <TelltaleEditor.hpp>
#include <UI/ApplicationUI.hpp>

// Run full application
static I32 Executor_Editor(const std::vector<CommandLine::TaskArgument>& args)
{
    ApplicationUI App{};
    return App.Run(args);
}

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv, &Executor_Editor);
#ifdef DEBUG
    Memory::DumpTrackedMemory();
#endif
    return exit;
}
