//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "BlueprintNodeTemplateEditor.h"

#include "CoreMinimal.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "NameSelectStructCustomization.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintTaskTemplate.h"
#include "BlueprintAITaskTemplate.h"
#include "BlueprintNodeTemplate.h"
#include "K2Node_Blueprint_Template.h"
#include "K2Node_ExtendConstructAiTask.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "AssetRegistry/ARFilter.h"


#define LOCTEXT_NAMESPACE "FBlueprintNodeTemplateEditorModule"

void FBlueprintNodeTemplateEditorModule::StartupModule()
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FBlueprintNodeTemplateEditorModule::OnFilesLoaded);
    AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FBlueprintNodeTemplateEditorModule::OnAssetRenamed);
    AssetRegistryModule.Get().OnInMemoryAssetDeleted().AddRaw(this, &FBlueprintNodeTemplateEditorModule::HandleAssetDeleted);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(
        "NameSelect",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNameSelectStructCustomization::MakeInstance));
//++CK
    _OnObjectPropertyChangedDelegateHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FBlueprintNodeTemplateEditorModule::OnObjectPropertyChanged);
//--CK
}

void FBlueprintNodeTemplateEditorModule::ShutdownModule()
{
    //if(FAssetRegistryModule* AssetRegistryModule = FModuleManager::LoadModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
    //{
    //	AssetRegistryModule->Get().OnAssetRemoved().RemoveAll(this);
    //}

//++CK
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(_OnObjectPropertyChangedDelegateHandle);
//--CK
}

void FBlueprintNodeTemplateEditorModule::OnFilesLoaded()
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FBlueprintNodeTemplateEditorModule::OnAssetAdded);

    RefreshClassActions();
}

void FBlueprintNodeTemplateEditorModule::RefreshClassActions() const
{
    TArray<FAssetData> AssetDataArr;
    const IAssetRegistry* AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprintTaskTemplate::StaticClass()));
    Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprintAITaskTemplate::StaticClass()));
    Filter.bRecursiveClasses = true;

    AssetRegistry->GetAssets(Filter, AssetDataArr);

    for (const FAssetData& AssetData : AssetDataArr)
    {
        if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
        {
            if (const UClass* TestClass = Blueprint->GeneratedClass)
            {
                if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()) || TestClass->IsChildOf(UBlueprintAITaskTemplate::StaticClass()))
                {
                    if (const auto CDO = TestClass->GetDefaultObject(true))
                    {
                        //check(CDO);
                        //if(CDO->GetLinkerCustomVersion(FBlueprintNodeTemplateCustomVersion::GUID) < FBlueprintNodeTemplateCustomVersion::LatestVersion)
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
        Bad->RefreshClassActions(UBlueprintTaskTemplate::StaticClass());
        Bad->RefreshClassActions(UK2Node_Blueprint_Template::StaticClass());

        Bad->RefreshClassActions(UBlueprintAITaskTemplate::StaticClass());
        Bad->RefreshClassActions(UK2Node_ExtendConstructAiTask::StaticClass());
    }
}

//++CK
auto
    FBlueprintNodeTemplateEditorModule::
    OnObjectPropertyChanged(
        UObject* ObjectBeingModified,
        FPropertyChangedEvent& PropertyChangedEvent) const
    -> void
{
    const auto& ObjectBeingModifiedBlueprint = Cast<UBlueprint>(ObjectBeingModified);
    if (IsValid(ObjectBeingModifiedBlueprint) == false)
    { return; }

    const auto& BlueprintParentClass = ObjectBeingModifiedBlueprint->GeneratedClass;
    if (IsValid(BlueprintParentClass) == false)
    { return; }

    constexpr auto CreateIfNeeded = true;
    if (auto* NodeTemplate = Cast<UBlueprintTaskTemplate>(BlueprintParentClass->GetDefaultObject(CreateIfNeeded));
        IsValid(NodeTemplate))
    {
        NodeTemplate->RefreshCollected();
    }
}
//--CK

void FBlueprintNodeTemplateEditorModule::OnAssetRenamed(const struct FAssetData& AssetData, const FString& Str) const
{
    OnAssetAdded(AssetData);
}

void FBlueprintNodeTemplateEditorModule::OnAssetAdded(const FAssetData& AssetData) const
{
    if (const auto Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
    {
        if (const UClass* TestClass = Blueprint->GeneratedClass)
        {
            if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()) || TestClass->IsChildOf(UBlueprintAITaskTemplate::StaticClass()))
            {
                //TestClass->GetDefaultObject(true);
                RefreshClassActions();
            }
        }
    }
}

void FBlueprintNodeTemplateEditorModule::HandleAssetDeleted(UObject* Object) const
{
    if (const auto Blueprint = Cast<UBlueprint>(Object))
    {
        if (const UClass* TestClass = Blueprint->ParentClass)
        {
            if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()) || TestClass->IsChildOf(UBlueprintAITaskTemplate::StaticClass()))
            {
                RefreshClassActions();
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintNodeTemplateEditorModule, BlueprintNodeTemplateEditor)
