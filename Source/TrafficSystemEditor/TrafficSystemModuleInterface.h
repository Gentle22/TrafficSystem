#pragma once

#include "Modules/ModuleManager.h"

class ITrafficSystemModuleListenerInterface
{
public:
    virtual ~ITrafficSystemModuleListenerInterface() = default;
    virtual void OnStartupModule() = 0;
    virtual void OnShutdownModule() = 0;
};

class ITrafficSystemModuleInterface : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (!IsRunningCommandlet())
        {
            AddModuleListener();
            for (auto ModuleListener : ModuleListeners)
            {
                ModuleListener->OnStartupModule();
            }
        }
    }

    virtual void ShutdownModule() override
    {
        for (auto ModuleListener : ModuleListeners)
        {
            ModuleListener->OnShutdownModule();
        }        
    }

    virtual void AddModuleListener() = 0;
    
protected:
    TArray<TSharedRef<ITrafficSystemModuleListenerInterface>> ModuleListeners; 
};