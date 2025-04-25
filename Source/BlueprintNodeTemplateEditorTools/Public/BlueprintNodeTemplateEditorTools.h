#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FBlueprintEditor;
class FWorkflowAllowedTabSet;

class FBlueprintNodeTemplateEditorToolsModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    FDelegateHandle BlueprintEditorTabSpawnerHandle, BlueprintEditorLayoutExtensionHandle;

    void RegisterTasksPaletteTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);

    void OnBlueprintCompiled();
};
