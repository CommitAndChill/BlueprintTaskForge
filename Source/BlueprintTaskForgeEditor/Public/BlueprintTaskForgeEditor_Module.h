#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FBlueprintEditor;
class FWorkflowAllowedTabSet;
class FString;
class UObject;
struct FAssetData;

class FBlueprintTaskForgeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FDelegateHandle BlueprintEditorTabSpawnerHandle, BlueprintEditorLayoutExtensionHandle;

private:

	void OnBlueprintCompiled();

	void OnFilesLoaded();

	void OnAssetAdded(const FAssetData& AssetData) const;
	void OnAssetRenamed(const FAssetData& AssetData, const FString& Str) const;
	void HandleAssetDeleted(UObject* Object) const;
	void RefreshClassActions() const;

//++CK
	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent) const;

private:
	FDelegateHandle _OnObjectPropertyChangedDelegateHandle;
//--CK
};
