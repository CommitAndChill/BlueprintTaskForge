#include "BlueprintTaskForgeEditor_Module.h"

#include "CoreMinimal.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "NameSelectStructCustomization.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintTaskTemplate.h"

#include "BlueprintTaskForge.h"
#include "K2Node_Blueprint_Template.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "AssetRegistry/ARFilter.h"
#include "NodeCustomizations/FBNTNodeDetailsCustomizations.h"


#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeEditorModule"

void FBlueprintTaskForgeEditorModule::StartupModule()
{
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	// AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnFilesLoaded);
	AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnAssetRenamed);
	AssetRegistryModule.Get().OnInMemoryAssetDeleted().AddRaw(this, &FBlueprintTaskForgeEditorModule::HandleAssetDeleted);

    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(
        "NameSelect",
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FNameSelectStructCustomization::MakeInstance));
//++CK
    _OnObjectPropertyChangedDelegateHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FBlueprintTaskForgeEditorModule::OnObjectPropertyChanged);
//--CK

	//BNT node customization
	PropertyModule.RegisterCustomClassLayout(
		"K2Node_Blueprint_Template",
		FOnGetDetailCustomizationInstance::CreateStatic(&FBNTNodeDetailsCustomizations::MakeInstance)
	);

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FBlueprintTaskForgeEditorModule::ShutdownModule()
{
	//if(FAssetRegistryModule* AssetRegistryModule = FModuleManager::LoadModulePtr<FAssetRegistryModule>(TEXT("AssetRegistry")))
	//{
	//	AssetRegistryModule->Get().OnAssetRemoved().RemoveAll(this);
	//}

	//++CK
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(_OnObjectPropertyChangedDelegateHandle);
	//--CK
}

void FBlueprintTaskForgeEditorModule::OnBlueprintCompiled()
{
	RefreshClassActions();
}

void FBlueprintTaskForgeEditorModule::OnFilesLoaded()
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnAssetAdded);

	RefreshClassActions();

#if WITH_EDITOR
	GEditor->OnBlueprintCompiled().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnBlueprintCompiled);
#endif
}

void FBlueprintTaskForgeEditorModule::RefreshClassActions() const
{
    TArray<FAssetData> AssetDataArr;
    const IAssetRegistry* AssetRegistry = &FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UBlueprintTaskTemplate::StaticClass()));
	Filter.bRecursiveClasses = true;

    AssetRegistry->GetAssets(Filter, AssetDataArr);

	for (const FAssetData& AssetData : AssetDataArr)
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
		{
			if (const UClass* TestClass = Blueprint->GeneratedClass)
			{
				if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()))
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
		Bad->RefreshClassActions(UBlueprintTaskTemplate::StaticClass());
		Bad->RefreshClassActions(UK2Node_Blueprint_Template::StaticClass());
	}
}

//++CK
auto
    FBlueprintTaskForgeEditorModule::
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

void FBlueprintTaskForgeEditorModule::OnAssetRenamed(const struct FAssetData& AssetData, const FString& Str) const
{
    OnAssetAdded(AssetData);
}

void FBlueprintTaskForgeEditorModule::OnAssetAdded(const FAssetData& AssetData) const
{
	if (const auto Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
	{
		if (const UClass* TestClass = Blueprint->GeneratedClass)
		{
			if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()))
			{
				//TestClass->GetDefaultObject(true);
				RefreshClassActions();
			}
		}
	}
}

void FBlueprintTaskForgeEditorModule::HandleAssetDeleted(UObject* Object) const
{
	if (const auto Blueprint = Cast<UBlueprint>(Object))
	{
		if (const UClass* TestClass = Blueprint->ParentClass)
		{
			if (TestClass->IsChildOf(UBlueprintTaskTemplate::StaticClass()))
			{
				RefreshClassActions();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintTaskForgeEditorModule, BlueprintTaskForgeEditor)
