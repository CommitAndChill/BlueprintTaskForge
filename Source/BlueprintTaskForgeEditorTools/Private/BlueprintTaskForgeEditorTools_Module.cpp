// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#include "BlueprintTaskForgeEditorTools_Module.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintEditorModule.h"
#include "BtfTaskForge.h"
#include "BtfTaskForge_K2Node.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#include "Tools/BtfSTaskPalette.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeEditorToolsModule"

void FBlueprintTaskForgeEditorToolsModule::StartupModule()
{
	/**V: Most of the code revolving the registration is copied from ActorSequenceEditor*/
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FBlueprintTaskForgeEditorToolsModule::RegisterTasksPaletteTab);

	/**V: For some reason, if this delegate is in the uncooked module, it will cause crashes.
	 * But this will essentially update the context menu and the task palette when a task blueprint is compiled. */
	GEditor->OnBlueprintCompiled().AddRaw(this, &FBlueprintTaskForgeEditorToolsModule::OnBlueprintCompiled);
}

void FBlueprintTaskForgeEditorToolsModule::ShutdownModule()
{

}

void FBlueprintTaskForgeEditorToolsModule::RegisterTasksPaletteTab(FWorkflowAllowedTabSet& TabFactories,
	FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	TabFactories.RegisterFactory(MakeShared<FTaskPaletteSummoner>(BlueprintEditor));
}

void FBlueprintTaskForgeEditorToolsModule::OnBlueprintCompiled()
{
	/**Refresh the asset actions. Copied from the uncooked module.*/

	TArray<FAssetData> AssetDataArr;
	const IAssetRegistry* AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBtf_TaskForge::StaticClass()));
	Filter.bRecursiveClasses = true;

	AssetRegistry->GetAssets(Filter, AssetDataArr);

	for (const FAssetData& AssetData : AssetDataArr)
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			if (const UClass* TestClass = Blueprint->GeneratedClass)
			{
				if (TestClass->IsChildOf(UBtf_TaskForge::StaticClass()))
				{
					if (const auto CDO = TestClass->GetDefaultObject(true))
					{
						//check(CDO);
						//if(CDO->GetLinkerCustomVersion(FBlueprintTaskForgeCustomVersion::GUID) < FBlueprintTaskForgeCustomVersion::LatestVersion)
						//{
						//	CDO->MarkPackageDirty();
						//}
					}
				}
			}
		}
	}

	if (FBlueprintActionDatabase* Bad = FBlueprintActionDatabase::TryGet())
	{
		Bad->RefreshClassActions(UBtf_TaskForge::StaticClass());
		Bad->RefreshClassActions(UBtf_TaskForge_K2Node::StaticClass());
	}
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FBlueprintTaskForgeEditorToolsModule, BlueprintTaskForgeEditorTools)