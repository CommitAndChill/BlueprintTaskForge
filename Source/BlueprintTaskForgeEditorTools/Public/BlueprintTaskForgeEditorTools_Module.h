#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintEditor;
class FWorkflowAllowedTabSet;

class FBlueprintTaskForgeEditorToolsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void RegisterTasksPaletteTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);

    void OnBlueprintCompiled();

private:
    FDelegateHandle BlueprintEditorTabSpawnerHandle;
    FDelegateHandle BlueprintEditorLayoutExtensionHandle;
};
