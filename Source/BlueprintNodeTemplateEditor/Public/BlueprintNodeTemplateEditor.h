//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FString;
class UObject;
struct FAssetData;

class FBlueprintNodeTemplateEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
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
