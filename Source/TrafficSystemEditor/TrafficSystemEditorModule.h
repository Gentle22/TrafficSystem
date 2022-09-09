#pragma once

#include "TrafficSystemModuleInterface.h"

class FTrafficSystemEditorModule : public ITrafficSystemModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    virtual void AddModuleListener() override;

    static inline FTrafficSystemEditorModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FTrafficSystemEditorModule>("TrafficSystemEditor");
    }

    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("TrafficSystemEditor");
    }
};