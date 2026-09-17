#include <TelltaleEditor.hpp>
#include <Application.hpp>

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv, &Application::RunApplication);
#ifdef DEBUG
    Memory::DumpTrackedMemory();
#endif
    return exit;
}
