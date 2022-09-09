#pragma once

#include "TrafficSystemEditor/TrafficSystemModuleInterface.h"

class TrafficSystemEdModeTool : public ITrafficSystemModuleListenerInterface,
                                public TSharedFromThis<TrafficSystemEdModeTool>
{
public:
    virtual void OnStartupModule() override;
    virtual void OnShutdownModule() override;

protected:
    void RegisterEditorMode();
    void UnregisterEditorMode();
};