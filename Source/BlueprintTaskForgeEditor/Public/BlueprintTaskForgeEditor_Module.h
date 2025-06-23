#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "BftMacros.h"

class FBlueprintEditor;
class FWorkflowAllowedTabSet;
class FString;
class UObject;
struct FAssetData;

// --------------------------------------------------------------------------------------------------------------------

class FBlueprintTaskForgeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void OnBlueprintCompiled();
    void OnFilesLoaded();
    void OnAssetAdded(const FAssetData& AssetData) const;
    void OnAssetRenamed(const FAssetData& AssetData, const FString& Str) const;
    void HandleAssetDeleted(UObject* Object) const;
    void RefreshClassActions() const;
    void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent) const;

private:
    FDelegateHandle OnObjectPropertyChangedDelegateHandle;
    FDelegateHandle OnAssetRenamedDelegateHandle;
    FDelegateHandle OnInMemoryAssetDeletedDelegateHandle;
    FDelegateHandle OnFilesLoadedDelegateHandle;
    FDelegateHandle BlueprintEditorTabSpawnerHandle;
    FDelegateHandle BlueprintEditorLayoutExtensionHandle;
};

// --------------------------------------------------------------------------------------------------------------------
