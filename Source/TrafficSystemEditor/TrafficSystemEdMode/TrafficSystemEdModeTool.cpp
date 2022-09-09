#include "TrafficSystemEdModeTool.h"

#include "EditorModeRegistry.h"
#include "TrafficSystemEdMode.h"

void TrafficSystemEdModeTool::OnStartupModule()
{
    RegisterEditorMode();    
}

void TrafficSystemEdModeTool::OnShutdownModule()
{
    UnregisterEditorMode();
}

void TrafficSystemEdModeTool::RegisterEditorMode()
{
    FEditorModeRegistry::Get().RegisterMode<FTrafficSystemEdMode>(FTrafficSystemEdMode::EM_TrafficSystemEdMode,
        FText::FromString("Traffic System"), FSlateIcon(), true, 500);
}

void TrafficSystemEdModeTool::UnregisterEditorMode()
{
    FEditorModeRegistry::Get().UnregisterMode(FTrafficSystemEdMode::EM_TrafficSystemEdMode);
}
