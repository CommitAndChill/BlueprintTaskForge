#include "BlueprintTaskForgeEditorTools_Module.h"
#include "BftMacros.h"

#include "BlueprintActionDatabase.h"
#include "BlueprintEditorModule.h"
#include "BtfTaskForge.h"
#include "BtfTaskForge_K2Node.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

#include "Tools/BtfSTaskPalette.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeEditorToolsModule"

void FBlueprintTaskForgeEditorToolsModule::StartupModule()
{
    auto& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
    BlueprintEditorTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FBlueprintTaskForgeEditorToolsModule::RegisterTasksPaletteTab);

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
    auto AssetDataArr = TArray<FAssetData>{};

    if (const auto* AssetRegistry = IAssetRegistry::Get();
        AssetRegistry != nullptr)
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(FTopLevelAssetPath(UBtf_TaskForge::StaticClass()));
        Filter.bRecursiveClasses = true;

        AssetRegistry->GetAssets(Filter, AssetDataArr);
    }

    for (const auto& AssetData : AssetDataArr)
    {
        if (const auto* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            if (const UClass* TestClass = Blueprint->GeneratedClass)
            {
                if (TestClass->IsChildOf(UBtf_TaskForge::StaticClass()))
                {
                    if (const auto CDO = TestClass->GetDefaultObject(true))
                    {
                    }
                }
            }
        }
    }

    if (auto* Bad = FBlueprintActionDatabase::TryGet())
    {
        Bad->RefreshClassActions(UBtf_TaskForge::StaticClass());
        Bad->RefreshClassActions(UBtf_TaskForge_K2Node::StaticClass());
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintTaskForgeEditorToolsModule, BlueprintTaskForgeEditorTools)