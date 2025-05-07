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

//++CK
//UBtf_TaskForge* UBtf_TaskForge::BlueprintTaskForge(UObject* Outer, const TSubclassOf<UBtf_TaskForge> Class)
UBtf_TaskForge* UBtf_TaskForge::BlueprintTaskForge(UObject* Outer, const TSubclassOf<UBtf_TaskForge> Class, FString NodeGuidStr)
//--CK
{
    if (IsValid(Outer) && Class && !Class->HasAnyClassFlags(CLASS_Abstract))
    {
        UBtf_TaskForge* TaskTemplate = GetTaskByNodeGUID(Outer, NodeGuidStr);

//++CK
        //const FName TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName()); //Class->GetFName();//
        const FName TaskObjName = MakeUniqueObjectName(Outer, Class, Class->GetFName(), EUniqueObjectNameOptions::GloballyUnique); //Class->GetFName();//
//--CK
        const auto Task = NewObject<UBtf_TaskForge>(Outer, Class, TaskObjName, RF_NoFlags, TaskTemplate ? TaskTemplate : nullptr);

//++CK
        if (IsValid(Task) == false)
        { return Task; }

        if (const auto& BlueprintTaskEngineSystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSystem))
        {
            BlueprintTaskEngineSystem->Request_Add(FGuid(NodeGuidStr), Task);
        }
//--CK

        return Task;
    }
    return nullptr;
}

UBtf_TaskForge* UBtf_TaskForge::GetTaskByNodeGUID(UObject* Outer, FString NodeGUID)
{
    for (UClass* TemplateOwnerClass = (Outer != nullptr) ? Outer->GetClass() : Outer->GetClass()
        ; TemplateOwnerClass
        ; TemplateOwnerClass = TemplateOwnerClass->GetSuperClass())
    {
        if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass))
        {
            if(UBlueprint* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy))
            {
                for(TObjectPtr<UBlueprintExtension> Extension : Blueprint->GetExtensions())
                {
                    if(Extension)
                    {
                        if(Extension->IsA<UBtf_TaskForge>() && Extension->GetFName().ToString().Contains(NodeGUID))
                        {
                            return Cast<UBtf_TaskForge>(Extension);
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

void
    UBtf_TaskForge::Activate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Activate)

    if (const auto World = GetWorld();
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->Request_TrackTask(this);
    }

    Activate_Internal();
}

void
    UBtf_TaskForge::Deactivate()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate)

    if(!IsActive)
    {
        return;
    }

    for (const auto& Task : _TasksToDeactivateOnDeactivate)
    {
        if (Task.IsValid())
        {
            Task->Deactivate();
        }
    }

    if (const auto World = GetWorld();
        IsValid(World))
    {
        World->GetSubsystem<UBtf_WorldSubsystem>()->Request_UntrackTask(this);
    }

    Deactivate_Internal();
}

//++CK
auto
    UBtf_TaskForge::
    DoRequest_AddTaskToDeactivateOnDeactivate(
        UBtf_TaskForge* InTask)
    -> void
{
    _TasksToDeactivateOnDeactivate.Emplace(InTask);
}
//--CK

//++CK
auto
    UBtf_TaskForge::
    OnDestroy()
    -> void
{
    IsBeingDestroyed = true;
    MarkAsGarbage();
}
//--CK

void UBtf_TaskForge::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
#if WITH_EDITOR
    Ar.UsingCustomVersion(FBlueprintTaskForgeCustomVersion::GUID);
    if (Ar.IsLoading() && GetLinkerCustomVersion(FBlueprintTaskForgeCustomVersion::GUID) < FBlueprintTaskForgeCustomVersion::ExposeOnSpawnInClass)
    {
        RefreshCollected();
        const FBtf_SpawnParam Spawn = UBtf_ExtendConstructObject_Utils::CollectSpawnParam(GetClass(), AllDelegates, AllFunctions, AllFunctionsExec, AllParam);
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

//++CK
void
    UBtf_TaskForge::Deactivate_Internal()
{
    QUICK_SCOPE_CYCLE_COUNTER(TaskNode_Deactivate_Internal)
    if (IsBeingDestroyed)
    { return; }

    if (IsValid(GetOuter()))
    {
        Deactivate_BP();
    }

    IsActive = false;

    if (IsValid(GEngine))
    {
        if (const auto& BlueprintTaskEngineSubsystem = GEngine->GetEngineSubsystem<UBtf_EngineSubsystem>();
            IsValid(BlueprintTaskEngineSubsystem))
        {
            BlueprintTaskEngineSubsystem->Request_Remove(this);
        }
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

//--CK

TArray<FString> UBtf_TaskForge::ValidateNodeDuringCompilation_Implementation()
{
    return TArray<FString>();
}

void UBtf_TaskForge::DeactivateAllTasksRelatedToObject(UObject* Object)
{
    TArray<UObject*> SubObjects;
    GetObjectsWithOuter(Object, SubObjects);

    for(auto& CurrentObject : SubObjects)
    {
        if(UBtf_TaskForge* TaskTemplate = Cast<UBtf_TaskForge>(CurrentObject))
        {
            TaskTemplate->Deactivate();
        }
    }
}

void UBtf_TaskForge::TriggerCustomOutputPin(FName OutputPin, TInstancedStruct<FCustomOutputPinData> Data)
{
    /**For now, this is just broadcasting the event. Future changes could include adding
     * logging into this function or validation. */
    OnCustomPinTriggered.Broadcast(OutputPin, Data);
}

TArray<FCustomOutputPin> UBtf_TaskForge::GetCustomOutputPins_Implementation()
{
    return TArray<FCustomOutputPin>();
}

TArray<FName> UBtf_TaskForge::GetCustomOutputPinNames()
{
    TArray<FName> Result;
    for(auto& CurrentPin : GetCustomOutputPins())
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
    ; TemplateOwnerClass
    ; TemplateOwnerClass = TemplateOwnerClass->GetOuter())
    {
        if(UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(TemplateOwnerClass))
        {
            if(UBlueprint* Blueprint = Cast<UBlueprint>(BPGC->ClassGeneratedBy))
            {
                return Blueprint->GetExtensions().Contains(this);
            }
        }

        if(UBlueprint* Blueprint = Cast<UBlueprint>(TemplateOwnerClass))
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
        const FProperty* Property = *PropertyIt;
        const bool bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const bool bIsExposedToSpawn = Property->HasMetaData(TEXT("ExposeOnSpawn")) || Property->HasAllPropertyFlags(CPF_ExposeOnSpawn);
        const bool bIsSettableExternally = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

        //++CK
        if (const auto& IsBlueprintPrivate = Property->GetMetaData(TEXT("BlueprintPrivate")) == TEXT("true"))
        { continue; }
        //--CK

        if (!Property->HasAnyPropertyFlags(CPF_Parm) && !bIsDelegate)
        {
            if (bIsExposedToSpawn && bIsSettableExternally && Property->HasAllPropertyFlags(CPF_BlueprintVisible))
            {
                Out.Add(Property->GetFName());
            }
            else if (
                !Property->HasAnyPropertyFlags(			 //
                    CPF_NativeAccessSpecifierProtected | //
                    CPF_NativeAccessSpecifierPrivate |	 //
                    CPF_Protected |						 //
                    CPF_BlueprintReadOnly |				 //
                    CPF_EditorOnly |					 //
                    CPF_InstancedReference |			 //
                    CPF_Deprecated |					 //
                    CPF_ExportObject) &&				 //
                Property->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
            {
                Out.Add(Property->GetFName());
            }
        }
    }
}
void UBtf_TaskForge::CollectFunctions(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<UField> It(InClass, EFieldIteratorFlags::IncludeSuper); It; ++It)
    {
        if (const UFunction* LocFunction = Cast<UFunction>(*It))
        {
            if (LocFunction->HasAllFunctionFlags(FUNC_BlueprintCallable | FUNC_Public) && //
                !LocFunction->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) && //
//++CK
                // Autocall functions should not display in the list of exec functions since they will
                // revert to 'None' on save anyway
                !LocFunction->GetBoolMetaData(FName(TEXT("ExposeAutoCall"))) &&           //
//--CK
                !LocFunction->HasMetaData(FName(TEXT("DeprecatedFunction"))) &&			  //
                !FObjectEditorUtils::IsFunctionHiddenFromClass(LocFunction, InClass) &&	  //!InClass->IsFunctionHidden(*LocFunction->GetName())
                !LocFunction->HasAnyFunctionFlags(										  //
                    FUNC_Static |														  //
                    FUNC_UbergraphFunction |											  //
                    FUNC_Delegate |														  //
                    FUNC_Private |														  //
                    FUNC_Protected |													  //
                    FUNC_EditorOnly |													  //
                    FUNC_BlueprintPure |												  //
                    FUNC_Const))														  // FUNC_BlueprintPure
            {
                Out.Add(LocFunction->GetFName());
            }
        }
    }
}
void UBtf_TaskForge::CollectDelegates(const UClass* InClass, TSet<FName>& Out)
{
    Out.Reset();
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        if (const FMulticastDelegateProperty* DelegateProperty = CastField<FMulticastDelegateProperty>(*PropertyIt))
        {
            if (DelegateProperty->HasAnyPropertyFlags(FUNC_Private | CPF_Protected | FUNC_EditorOnly) || //
                !DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable) ||						 //
                DelegateProperty->GetBoolMetaData(FName(TEXT("BlueprintInternalUseOnly"))) ||			 //
                DelegateProperty->HasMetaData(TEXT("DeprecatedFunction")))
            {
                continue;
            }

            if (const UFunction* LocFunction = DelegateProperty->SignatureFunction)
            {
                if (!LocFunction->HasAllFunctionFlags(FUNC_Public) ||				  //
                    LocFunction->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")) || //
                    LocFunction->HasMetaData(TEXT("DeprecatedFunction")) ||
                    LocFunction->HasAnyFunctionFlags( //
                        FUNC_Static |				  //
                        FUNC_BlueprintPure |		  //
                        FUNC_Const |				  //
                        FUNC_UbergraphFunction |	  //
                        FUNC_Private |				  //
                        FUNC_Protected |			  //
                        FUNC_EditorOnly))
                {
                    continue;
                }
            }
            Out.Add(DelegateProperty->GetFName());
        }
    }
}
void UBtf_TaskForge::CleanInvalidParams(TArray<FBtf_NameSelect>& Arr, const TSet<FName>& ArrRef)
{
    for (int32 i = Arr.Num() - 1; i >= 0; --i)
    {
        if ((Arr[i].Name != NAME_None && !ArrRef.Contains(Arr[i])) || Arr[i].Name == GET_MEMBER_NAME_CHECKED(UBtf_TaskForge, OnCustomPinTriggered))
        {
            Arr.RemoveAt(i, 1, EAllowShrinking::No);
        }
    }
}

void UBtf_TaskForge::RefreshCollected()
{

    #if WITH_EDITORONLY_DATA
    {
        const UClass* InClass = GetClass();
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

//++CK
        AutoCallFunction.Remove(FName(TEXT("Deactivate")));
        //++V ExecFunction.AddUnique was added by CK, I am adding the developer settings logic to optionally disable this behavior
        if(const UBtf_RuntimeSettings* DeveloperSettings = GetDefault<UBtf_RuntimeSettings>())
        {
            if(DeveloperSettings->EnforceDeactivateExecFunction)
            {
                ExecFunction.AddUnique(FName(TEXT("Deactivate")));
            }
        }
        //--V
//--CK
    }
    #endif // WITH_EDITORONLY_DATA
}

void UBtf_TaskForge::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    RefreshCollected();
    Super::PostEditChangeProperty(PropertyChangedEvent);

    OnPostPropertyChanged.ExecuteIfBound(PropertyChangedEvent);
}

//++CK


FString UBtf_TaskForge::Get_StatusString_Implementation() const
{
    return FString();
}

auto
    UBtf_TaskForge::
    Get_IsActive() const
    -> bool
{
    return IsActive;
}

//--CK

#endif // WITH_EDITOR
