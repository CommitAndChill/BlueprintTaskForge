#include "BtfTaskForge.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "BlueprintTaskForge_Module.h"
#include "BtfExtendConstructObject_Utils.h"
#include "Subsystem/BtfSubsystem.h"

#include "Settings/BtfRuntimeSettings.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#include "Subsystem/BtfSubsystem.h"
#endif // WITH_EDITOR

UBtf_TaskForge::UBtf_TaskForge(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

UBtf_TaskForge* UBtf_TaskForge::BlueprintTaskForge(UObject* Outer, const TSubclassOf<UBtf_TaskForge> Class, FString NodeGuidStr)
{
    if (NOT IsValid(Outer)) { return nullptr; }
    if (NOT Class) { return nullptr; }
    if (Class->HasAnyClassFlags(CLASS_Abstract)) { return nullptr; }

    auto* TaskTemplate = GetTaskByNodeGUID(Outer, NodeGuidStr);
    const auto TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName(), EUniqueObjectNameOptions::GloballyUnique);
    auto* Task = NewObject<UBtf_TaskForge>(Outer, Class, TaskObjName, RF_NoFlags, TaskTemplate ? TaskTemplate : nullptr);

    if (NOT IsValid(Task))
    {
        return Task;
    }

    if (const auto* BlueprintTaskEngineSystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>(); 
        IsValid(BlueprintTaskEngineSystem))
    {
        BlueprintTaskEngineSystem->Request_Add(FGuid(NodeGuidStr), Task);
    }

    return Task;
}

UBtf_TaskForge* UBtf_TaskForge::GetTaskByNodeGUID(UObject* Outer, FString NodeGUID)
{
    for (UClass* TemplateOwnerClass = (Outer != nullptr) ? Outer->GetClass() : Outer->GetClass()
        ; TemplateOwnerClass != nullptr
        ; TemplateOwnerClass = TemplateOwnerClass->GetSuperClass())
    {
        auto* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass);
        if (NOT IsValid(BPGC)) { continue; }

        auto* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy);
        if (NOT IsValid(Blueprint)) { continue; }

        for (const auto& Extension : Blueprint->GetExtensions())
        {
            if (NOT IsValid(Extension)) { continue; }

            if (Extension->IsA<UBtf_TaskForge>() && Extension->GetFName().ToString().Contains(NodeGUID))
            {
                return Cast<UBtf_TaskForge>(Extension);
            }
        }
    }

    return nullptr;
}

void UBtf_TaskForge::Activate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Activate)

    if (const auto* World = GetWorld(); 
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->Request_TrackTask(this);
    }

    Activate_Internal();
}

void UBtf_TaskForge::Deactivate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate)

    if (NOT IsActive) { return; }

    for (const auto& Task : TasksToDeactivateOnDeactivate)
    {
        if (Task.IsValid())
        {
            Task->Deactivate();
        }
    }

    if (const auto* World = GetWorld(); 
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->Request_UntrackTask(this);
    }

    Deactivate_Internal();
}

UWorld* UBtf_TaskForge::GetWorld() const
{
    if (IsTemplate()) { return nullptr; }
    if (NOT GetOuter()) { return nullptr; }

    return GetOuter()->GetWorld();
}

void UBtf_TaskForge::OnDestroy()
{
    IsBeingDestroyed = true;
    MarkAsGarbage();
}

void UBtf_TaskForge::Activate_Internal()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Activate_Internal)

    if (IsBeingDestroyed) { return; }
    if (NOT IsValid(GetOuter())) { return; }

    SetupAutomaticCleanup();
    IsActive = true;
    Activate_BP();
}

void UBtf_TaskForge::SetupAutomaticCleanup()
{
    if (NOT GetOuter()) { return; }

    if (auto* Actor = Cast<AActor>(GetOuter()); 
        IsValid(Actor))
    {
        Actor->OnDestroyed.AddDynamic(this, &UBtf_TaskForge::OnActorOuterDestroyed);
        return;
    }

    if (auto* TaskTemplate = Cast<UBtf_TaskForge>(GetOuter()); 
        IsValid(TaskTemplate))
    {
        TaskTemplate->TrackTaskForAutomaticDeactivation(this);
    }
}

void UBtf_TaskForge::OnActorOuterDestroyed(AActor* Actor)
{
    Deactivate();
}

void UBtf_TaskForge::TrackTaskForAutomaticDeactivation(UBtf_TaskForge* Task)
{
    if (NOT IsValid(Task)) { return; }
    if (TasksToDeactivateOnDeactivate.Contains(Task)) { return; }

    TasksToDeactivateOnDeactivate.Add(Task);
}

void UBtf_TaskForge::UntrackTaskForAutomaticDeactivation(UBtf_TaskForge* Task)
{
    if (NOT IsValid(Task)) { return; }
    if (NOT TasksToDeactivateOnDeactivate.Contains(Task)) { return; }

    TasksToDeactivateOnDeactivate.RemoveSingle(Task);
}

void UBtf_TaskForge::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
#if WITH_EDITOR
    Ar.UsingCustomVersion(FBlueprintTaskForgeCustomVersion::GUID);
    if (Ar.IsLoading() && GetLinkerCustomVersion(FBlueprintTaskForgeCustomVersion::GUID) < FBlueprintTaskForgeCustomVersion::ExposeOnSpawnInClass)
    {
        RefreshCollected();
        const auto& Spawn = UBtf_ExtendConstructObject_Utils::CollectSpawnParam(GetClass(), AllDelegates, AllFunctions, AllFunctionsExec, AllParam);
        for (const auto& It : Spawn.AutoCallFunction)
        {
            AutoCallFunction.AddUnique(It);
        }
        for (const auto& It : Spawn.ExecFunction)
        {
            ExecFunction.AddUnique(It);
        }
        for (const auto& It : Spawn.InDelegate)
        {
            InDelegate.AddUnique(It);
        }
        for (const auto& It : Spawn.OutDelegate)
        {
            OutDelegate.AddUnique(It);
        }
        for (const auto& It : Spawn.SpawnParam)
        {
            SpawnParam.AddUnique(It);
        }
    }
#endif
}

void UBtf_TaskForge::Deactivate_Internal()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate_Internal)
    
    if (IsBeingDestroyed) { return; }

    if (IsValid(GetOuter()))
    {
        Deactivate_BP();
    }

    IsActive = false;

    if (const auto* BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
        IsValid(BlueprintTaskEngineSubsystem))
    {
        BlueprintTaskEngineSubsystem->Request_Remove(this);
    }

    OnDestroy();
}

bool UBtf_TaskForge::Get_StatusBackgroundColor_Implementation(FLinearColor& OutColor) const
{
    OutColor = FLinearColor();
    return false;
}

FString UBtf_TaskForge::Get_NodeDescription_Implementation() const
{
    return FString();
}

TArray<FString> UBtf_TaskForge::ValidateNodeDuringCompilation_Implementation()
{
    return TArray<FString>();
}

void UBtf_TaskForge::DeactivateAllTasksRelatedToObject(UObject* Object)
{
    auto SubObjects = TArray<UObject*>{};
    GetObjectsWithOuter(Object, SubObjects);

    for (auto& CurrentObject : SubObjects)
    {
        if (auto* TaskTemplate = Cast<UBtf_TaskForge>(CurrentObject); 
            IsValid(TaskTemplate))
        {
            TaskTemplate->Deactivate();
        }
    }
}

void UBtf_TaskForge::TriggerCustomOutputPin(FName OutputPin, TInstancedStruct<FCustomOutputPinData> Data)
{
    OnCustomPinTriggered.Broadcast(OutputPin, Data);
}

TArray<FCustomOutputPin> UBtf_TaskForge::GetCustomOutputPins_Implementation()
{
    return TArray<FCustomOutputPin>();
}

TArray<FName> UBtf_TaskForge::GetCustomOutputPinNames()
{
    auto Result = TArray<FName>{};
    for (auto& CurrentPin : GetCustomOutputPins())
    {
        Result.Add(FName(CurrentPin.PinName));
    }
    return Result;
}

bool UBtf_TaskForge::GetNodeTitleColor_Implementation(FLinearColor& Color)
{
    Color = FLinearColor();
    return false;
}

bool UBtf_TaskForge::IsExtension() const
{
    for (UObject* TemplateOwnerClass = (GetOuter() != nullptr) ? GetOuter() : nullptr
    ; TemplateOwnerClass != nullptr
    ; TemplateOwnerClass = TemplateOwnerClass->GetOuter())
    {
        if (auto* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass); 
            IsValid(BPGC))
        {
            if (auto* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy); 
                IsValid(Blueprint))
            {
                return Blueprint->GetExtensions().Contains(this);
            }
        }

        if (auto* Blueprint = Cast<UBlueprint>(TemplateOwnerClass); 
            IsValid(Blueprint))
        {
            return Blueprint->GetExtensions().Contains(this);
        }
    }

    return false;
}

#if WITH_EDITOR
void UBtf_TaskForge::CollectSpawnParam(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        const auto* Property = *PropertyIt;
        const auto IsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const auto IsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
        const auto IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        if (Property->GetMetaData(TEXT("BlueprintPrivate")) == TEXT("true"))
        {
            continue;
        }

        if (Property->HasAnyPropertyFlags(CPF_Parm) || IsDelegate)
        {
            continue;
        }

        if (IsExposedToSpawn && IsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
        {
            Out.Add(Property->GetFName());
        }
        else if (
            NOT Property->HasAnyPropertyFlags(
                CPF_NativeAccessSpecifierProtected |
                CPF_NativeAccessSpecifierPrivate |
                CPF_Protected |
                CPF_BlueprintReadOnly |
                CPF_EditorOnly |
                CPF_InstancedReference |
                CPF_Deprecated |
                CPF_ExportObject) &&
            Property->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
        {
            Out.Add(Property->GetFName());
        }
    }
}

void UBtf_TaskForge::CollectFunctions(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<UField> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        const auto* LocFunction = Cast<UFunction>(*It);
        if (NOT IsValid(LocFunction)) { continue; }

        const auto HasRequiredFlags = LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public);
        const auto HasInternalUseOnly = LocFunction->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly")));
        const auto HasExposeAutoCall = LocFunction->GetBoolMetaData(FName(TEXT("ExposeAutoCall")));
        const auto HasDeprecated = LocFunction->HasMetaData(FName(TEXT("DeprecatedFunction")));
        const auto IsHidden = FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass);
        const auto HasExcludedFlags = LocFunction->HasAnyFunctionFlags(
            FUNC_Static |
            FUNC_UbergraphFunction |
            FUNC_Delegate |
            FUNC_Private |
            FUNC_Protected |
            FUNC_EditorOnly |
            FUNC_BlueprintPure |
            FUNC_Const);

        if (NOT HasRequiredFlags || HasInternalUseOnly || HasExposeAutoCall || HasDeprecated || IsHidden || HasExcludedFlags) { continue; }

        Out.Add(LocFunction->GetFName());
    }
}

void UBtf_TaskForge::CollectDelegates(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        const auto* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt);
        if (NOT DelegateProperty) { continue; }

        const auto HasInvalidFlags = DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly);
        const auto HasAssignableFlag = DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable);
        const auto HasInternalUseOnly = DelegateProperty->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly")));
        const auto HasDeprecated = DelegateProperty->HasMetaData(TEXT("DeprecatedFunction"));

        if (HasInvalidFlags || NOT HasAssignableFlag || HasInternalUseOnly || HasDeprecated) { continue; }

        if (const auto* LocFunction = DelegateProperty->SignatureFunction; 
            IsValid(LocFunction))
        {
            const auto HasPublicFlag = LocFunction->HasAllFunctionFlags(FUNC_Public);
            const auto HasFunctionInternalUseOnly = LocFunction->GetBoolMetaData(TEXT("BlueprintInternalUseOnly"));
            const auto HasFunctionDeprecated = LocFunction->HasMetaData(TEXT("DeprecatedFunction"));
            const auto HasInvalidFunctionFlags = LocFunction->HasAnyFunctionFlags(
                FUNC_Static |
                FUNC_BlueprintPure |
                FUNC_Const |
                FUNC_UbergraphFunction |
                FUNC_Private |
                FUNC_Protected |
                FUNC_EditorOnly);

            if (NOT HasPublicFlag || HasFunctionInternalUseOnly || HasFunctionDeprecated || HasInvalidFunctionFlags) { continue; }
        }

        Out.Add(DelegateProperty->GetFName());
    }
}

void UBtf_TaskForge::CleanInvalidParams(TArray<FBtf_NameSelect>& Arr, const TSet<FName>& ArrRef)
{
    for (int32 i = Arr.Num() - 1; i >= 0; --i)
    {
        const auto& CurrentItem = Arr[i];
        const auto IsInvalidParam = (CurrentItem.Get_Name() != NAME_None && NOT ArrRef.Contains(CurrentItem));
        const auto IsCustomPinTriggered = (CurrentItem.Get_Name() == GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered));

        if (IsInvalidParam || IsCustomPinTriggered)
        {
            Arr.RemoveAt(i, 1, EAllowShrinking::No);
        }
    }
}

void UBtf_TaskForge::RefreshCollected()
{
#if WITH_EDITORONLY_DATA
    const auto* InClass = GetClass();
    CollectSpawnParam(InClass, AllParam);
    CollectFunctions(InClass, AllFunctions);
    CollectDelegates(InClass, AllDelegates);
    AllFunctionsExec = AllFunctions;
    CleanInvalidParams(AutoCallFunction, AllFunctions);
    CleanInvalidParams(ExecFunction, AllFunctions);
    CleanInvalidParams(InDelegate, AllDelegates);
    CleanInvalidParams(OutDelegate, AllDelegates);
    CleanInvalidParams(SpawnParam, AllParam);
    AutoCallFunction.AddUnique(FName(TEXT("Activate")));
    
    for (auto& It : AutoCallFunction)
    {
        It.SetAllExclude(AllFunctions, AutoCallFunction);
        ExecFunction.Remove(It);
    }
    for (auto& It : ExecFunction)
    {
        It.SetAllExclude(AllFunctionsExec, ExecFunction);
        AutoCallFunction.Remove(It);
    }
    for (auto& It : InDelegate)
    {
        It.SetAllExclude(AllDelegates, InDelegate);
        OutDelegate.Remove(It);
    }
    for (auto& It : OutDelegate)
    {
        It.SetAllExclude(AllDelegates, OutDelegate);
        InDelegate.Remove(It);
    }
    for (auto& It : SpawnParam)
    {
        It.SetAllExclude(AllParam, SpawnParam);
    }
    
    AutoCallFunction.AddUnique(FName(TEXT("Activate")));
    AutoCallFunction.Remove(FName(TEXT("Deactivate")));
    
    if (const auto* DeveloperSettings = GetDefault<UBtf_RuntimeSettings>(); 
        IsValid(DeveloperSettings) && DeveloperSettings->Get_EnforceDeactivateExecFunction())
    {
        ExecFunction.AddUnique(FName(TEXT("Deactivate")));
    }
#endif // WITH_EDITORONLY_DATA
}

void UBtf_TaskForge::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RefreshCollected();
    Super::PostEditChangeProperty(PropertyChangedEvent);

    OnPostPropertyChanged.ExecuteIfBound(PropertyChangedEvent);
}

FString UBtf_TaskForge::Get_StatusString_Implementation() const
{
    return FString();
}

#endif // WITH_EDITOR