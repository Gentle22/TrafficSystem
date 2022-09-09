#include "TrafficSystemEditorModule.h"
#include "TrafficSystemEdMode/TrafficSystemEdModeTool.h"

IMPLEMENT_MODULE(FTrafficSystemEditorModule, TrafficSystemEditor)

void FTrafficSystemEditorModule::StartupModule()
{
    ITrafficSystemModuleInterface::StartupModule();
}

void FTrafficSystemEditorModule::ShutdownModule()
{
    ITrafficSystemModuleInterface::ShutdownModule();
}

void FTrafficSystemEditorModule::AddModuleListener()
{
    // Add tools later
    ModuleListeners.Add(MakeShareable(new TrafficSystemEdModeTool));
}


