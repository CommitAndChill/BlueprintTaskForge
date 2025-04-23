//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "NameSelect.h"

//++CK
#include "Subsystem/BlueprintTask_Subsystem.h"
//--CK

#include "UObject/Object.h"

#include "BlueprintTaskTemplate.generated.h"


class UWorld;

/** BlueprintTaskTemplate */
UCLASS(Abstract, Blueprintable, BlueprintType)
class BLUEPRINTNODETEMPLATE_API UBlueprintTaskTemplate : public UObject
{
    GENERATED_BODY()
public:
    UBlueprintTaskTemplate(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(
        BlueprintCallable,
        Category = "BlueprintTaskTemplate",
        meta =
            (DisplayName = "BlueprintTaskTemplate",
             DefaultToSelf = "Outer",
             BlueprintInternalUseOnly = "TRUE",
             DeterminesOutputType = "Class",
             Keywords = "BP Blueprint Task Template"))
//++CK
    //static UBlueprintTaskTemplate* BlueprintTaskTemplate(UObject* Outer, TSubclassOf<UBlueprintTaskTemplate> Class);
    static UBlueprintTaskTemplate* BlueprintTaskTemplate(UObject* Outer, TSubclassOf<UBlueprintTaskTemplate> Class, FString NodeGuidStr);
//--CK

    UFUNCTION(BlueprintCallable, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Activate", ExposeAutoCall = "true"))
//++CK
    void Activate()
    {
        if (const auto World = GetWorld();
            IsValid(World))
        {
            UUtils_BlueprintTask_Subsystem_UE::Request_TrackTask(World, this);
        }
        Activate_Internal();
    }
//--CK

//++CK
    UFUNCTION(BlueprintCallable, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Deactivate", ExposeAutoCall = "false"))
    void Deactivate()
    {
        QUICK_SCOPE_CYCLE_COUNTER(Deactivate)
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
            UUtils_BlueprintTask_Subsystem_UE::Request_UntrackTask(World, this);
        }
        Deactivate_Internal();
    }

    UFUNCTION(BlueprintCallable,
              Category = "BlueprintTaskTemplate",
              DisplayName = "[BlueprintNodeTemp] Add Task To Deactivate On  Deactivate",
              meta = (CompactNodeTitle="TaskToDeactivate_OnDeactivate", HideSelfPin = true, Keywords = "Register, Track"))
    void
    DoRequest_AddTaskToDeactivateOnDeactivate(
        class UBlueprintTaskTemplate* InTask);
//--CK

    virtual UWorld* GetWorld() const override { return WorldPrivate; }
    virtual void BeginDestroy() override;

//++CK
    virtual void OnDestroy();
//--CK

    virtual void Serialize(FArchive& Ar) override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Activate"))
    void Activate_BP();
    virtual void Activate_Internal()
    {
//++CK
        if (IsBeingDestroyed)
        { return; }

        if (IsValid(GetOuter()))
//--CK
        {
            IsActive = true;
            Activate_BP();
        }
    }

//++CK
    UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Deactivate"))
    void Deactivate_BP();
    virtual void Deactivate_Internal();

    // Short summary of node's content - displayed over node as NodeInfoPopup
    UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Get Node Description"))
    FString Get_NodeDescription_BP() const;

    // Information displayed while node is working - displayed over node as NodeInfoPopup
    UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Get Status String"))
    FString Get_StatusString_BP() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "BlueprintTaskTemplate", meta = (DisplayName = "Get Status Background Color"))
    bool Get_StatusBackgroundColor_BP(FLinearColor& OutColor) const;
//--CK

private:
    UPROPERTY(Transient)
    UWorld* WorldPrivate = nullptr;

//++CK
    UPROPERTY(Transient)
    bool IsBeingDestroyed = false;

    UPROPERTY(Transient)
    bool IsActive = false;

public:
    UPROPERTY(EditDefaultsOnly, Category = "DisplayOptions")
    FName Category = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "DisplayOptions")
    FName Tooltip = NAME_None;

    UPROPERTY(EditDefaultsOnly, Category = "DisplayOptions")
    FName MenuDisplayName = NAME_None;
//--CK

#if WITH_EDITOR
public:
    // force refresh all for old/new in-editor params/functions etc
    void RefreshCollected();
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

//++CK
    virtual auto
    Get_StatusBackgroundColor(
        FLinearColor& OutColor) const -> bool;

    virtual FString
    Get_NodeDescription() const;

    virtual FString
    Get_StatusString() const;

    auto
    Get_IsActive() const -> bool;
//--CK

protected:
    static void CollectSpawnParam(const UClass* InClass, TSet<FName>& Out);
    static void CollectFunctions(const UClass* InClass, TSet<FName>& Out);
    static void CollectDelegates(const UClass* InClass, TSet<FName>& Out);
    static void CleanInvalidParams(TArray<FNameSelect>& Arr, const TSet<FName>& ArrRef);

#endif // WITH_EDITOR

#if WITH_EDITORONLY_DATA
public:
    UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
    TSet<FName> AllDelegates;
    UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
    TSet<FName> AllFunctions;
    UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
    TSet<FName> AllFunctionsExec;
    UPROPERTY(VisibleDefaultsOnly, Category = "ExposeOptions")
    TSet<FName> AllParam;

    UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
    TArray<FNameSelect> SpawnParam;
    UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
    TArray<FNameSelect> AutoCallFunction;
    UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
    TArray<FNameSelect> ExecFunction;
    UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
    TArray<FNameSelect> InDelegate;
    UPROPERTY(EditDefaultsOnly, Category = "ExposeOptions")
    TArray<FNameSelect> OutDelegate;
#endif

//++CK
private:
    TArray<TWeakObjectPtr<UBlueprintTaskTemplate>> _TasksToDeactivateOnDeactivate;
//--CK
};
