#include "BlueprintTaskForgeEditor_Module.h"
#include "CoreMinimal.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BtfTaskForge.h"
#include "BtfTaskForge_K2Node.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "AssetRegistry/ARFilter.h"
#include "NodeCustomizations/BtfNameSelectStructCustomization.h"
#include "NodeCustomizations/BtfNodeDetailsCustomizations.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FBlueprintTaskForgeEditorModule"

void FBlueprintTaskForgeEditorModule::StartupModule()
{
    if (auto* AssetRegistry = IAssetRegistry::Get())
    {
        OnAssetRenamedDelegateHandle = AssetRegistry->OnAssetRenamed().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnAssetRenamed);
        OnInMemoryAssetDeletedDelegateHandle = AssetRegistry->OnInMemoryAssetDeleted().AddRaw(this, &FBlueprintTaskForgeEditorModule::HandleAssetDeleted);
        OnFilesLoadedDelegateHandle = AssetRegistry->OnFilesLoaded().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnFilesLoaded);
    }

    auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout(
        FBtf_NameSelect::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBtf_NameSelectStructCustomization::MakeInstance));

    OnObjectPropertyChangedDelegateHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FBlueprintTaskForgeEditorModule::OnObjectPropertyChanged);

    PropertyModule.RegisterCustomClassLayout(
        UBtf_TaskForge_K2Node::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FBtf_NodeDetailsCustomizations::MakeInstance)
    );

    PropertyModule.NotifyCustomizationModuleChanged();
}

void FBlueprintTaskForgeEditorModule::ShutdownModule()
{
    if (auto* AssetRegistry = IAssetRegistry::Get())
    {
        AssetRegistry->OnAssetRenamed().Remove(OnAssetRenamedDelegateHandle);
        AssetRegistry->OnInMemoryAssetDeleted().Remove(OnInMemoryAssetDeletedDelegateHandle);
        AssetRegistry->OnFilesLoaded().Remove(OnFilesLoadedDelegateHandle);
    }

    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(OnObjectPropertyChangedDelegateHandle);
}

void FBlueprintTaskForgeEditorModule::OnBlueprintCompiled()
{
    RefreshClassActions();
}

void FBlueprintTaskForgeEditorModule::OnFilesLoaded()
{
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnAssetAdded);

    RefreshClassActions();

#if WITH_EDITOR
    GEditor->OnBlueprintCompiled().AddRaw(this, &FBlueprintTaskForgeEditorModule::OnBlueprintCompiled);
#endif
}

void FBlueprintTaskForgeEditorModule::RefreshClassActions() const
{
    auto AssetDataArr = TArray<FAssetData>{};

    if (const auto* AssetRegistry = IAssetRegistry::Get())
    {
        auto Filter = FARFilter{};
        Filter.ClassPaths.Add(FTopLevelAssetPath(UBtf_TaskForge::StaticClass()));
        Filter.bRecursiveClasses = true;

        AssetRegistry->GetAssets(Filter, AssetDataArr);
    }

    for (const auto& AssetData : AssetDataArr)
    {
        const auto* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
        if (NOT IsValid(Blueprint))
        { continue; }

        const auto TestClass = Blueprint->GeneratedClass;
        if (NOT IsValid(TestClass))
        { continue; }

        if (NOT TestClass->IsChildOf(UBtf_TaskForge::StaticClass()))
        { continue; }

        if (const auto CDO = TestClass->GetDefaultObject(true);
            CDO != nullptr)
        {
            // CDO validation can be added here if needed
        }
    }

    if (auto* Bad = FBlueprintActionDatabase::TryGet())
    {
        Bad->RefreshClassActions(UBtf_TaskForge::StaticClass());
        Bad->RefreshClassActions(UBtf_TaskForge_K2Node::StaticClass());
    }
}

void FBlueprintTaskForgeEditorModule::OnObjectPropertyChanged(
    UObject* ObjectBeingModified,
    FPropertyChangedEvent& PropertyChangedEvent) const
{
    const auto* ObjectBeingModifiedBlueprint = Cast<UBlueprint>(ObjectBeingModified);
    if (NOT IsValid(ObjectBeingModifiedBlueprint))
    { return; }

    const auto BlueprintParentClass = ObjectBeingModifiedBlueprint->GeneratedClass;
    if (NOT IsValid(BlueprintParentClass))
    { return; }

    constexpr auto CreateIfNeeded = true;
    if (auto* NodeTemplate = Cast<UBtf_TaskForge>(BlueprintParentClass->GetDefaultObject(CreateIfNeeded));
        IsValid(NodeTemplate))
    {
        NodeTemplate->RefreshCollected();
    }
}

void FBlueprintTaskForgeEditorModule::OnAssetRenamed(const struct FAssetData& AssetData, const FString& Str) const
{
    OnAssetAdded(AssetData);
}

void FBlueprintTaskForgeEditorModule::OnAssetAdded(const FAssetData& AssetData) const
{
    const auto* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
    if (NOT IsValid(Blueprint))
    { return; }

    const auto TestClass = Blueprint->GeneratedClass;
    if (NOT IsValid(TestClass))
    { return; }

    if (NOT TestClass->IsChildOf(UBtf_TaskForge::StaticClass()))
    { return; }

    RefreshClassActions();
}

void FBlueprintTaskForgeEditorModule::HandleAssetDeleted(UObject* Object) const
{
    const auto* Blueprint = Cast<UBlueprint>(Object);
    if (NOT IsValid(Blueprint))
    { return; }

    const auto TestClass = Blueprint->ParentClass;
    if (NOT IsValid(TestClass))
    { return; }

    if (NOT TestClass->IsChildOf(UBtf_TaskForge::StaticClass()))
    { return; }

    RefreshClassActions();
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FBlueprintTaskForgeEditorModule, BlueprintTaskForgeEditor)
