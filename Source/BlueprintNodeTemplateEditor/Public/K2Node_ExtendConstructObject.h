//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "UObject/ObjectMacros.h"
#include "NameSelect.h"

//++CK
#include "K2Node_DynamicCast.h"
//--CK

#include "K2Node_ExtendConstructObject.generated.h"

class FBlueprintActionDatabaseRegistrar;
class UEdGraph;
class UEdGraphPin;
class UEdGraphSchema;
class UEdGraphSchema_K2;


/**
* UK2Node_ExtendConstructObject
*/
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_ExtendConstructObject : public UK2Node
{
    GENERATED_BODY()
public:
    UK2Node_ExtendConstructObject(const FObjectInitializer& ObjectInitializer);

    // - UEdGraphNode interface			// UK2Node_BaseAsyncTask
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
    virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
    virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
    virtual bool ShouldShowNodeProperties() const override { return true; }
    virtual bool CanDuplicateNode() const override { return true; }
    virtual void PostPasteNode() override;
    virtual UObject* GetJumpTargetForDoubleClick() const override;
    // End of UEdGraphNode interface	// UK2Node_BaseAsyncTask

    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    //virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const override;
    // - End of UEdGraphNode interface

    // - UK2Node interface
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
    virtual FName GetCornerIcon() const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;

    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override { return false; }
    virtual void ReconstructNode() override;
    virtual void PostReconstructNode() override;
    virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)
        const override;

    virtual bool UseWorldContext() const;
    virtual FString GetPinMetaData(FName InPinName, FName InKey) override;
    // - End of UK2Node interface

    virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(FPropertyChangedEvent& e) override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "ExposeOptions")
    void ResetToDefaultExposeOptions() { ResetToDefaultExposeOptions_Impl(); }
    virtual void ResetToDefaultExposeOptions_Impl();
#endif

    UEdGraphPin* GetClassPin() const { return FindPin(ClassPinName); }
    UEdGraphPin* GetWorldContextPin() const;

protected:
    UPROPERTY()
    FName WorldContextPinName = FName(TEXT("Outer"));
    UPROPERTY()
    FName ClassPinName = FName(TEXT("Class"));
    UPROPERTY()
    FName OutPutObjectPinName = FName(TEXT("Object"));

    UPROPERTY()
    UClass* ProxyFactoryClass;
    UPROPERTY()
    UClass* ProxyClass;
    UPROPERTY()
    FName ProxyFactoryFunctionName;

    UPROPERTY()
    TSet<FName> AllDelegates;
    UPROPERTY()
    TSet<FName> AllFunctions;
    UPROPERTY()
    TSet<FName> AllFunctionsExec;
    UPROPERTY()
    TSet<FName> AllParam;

    UPROPERTY()
    bool bSelfContext = false;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FNameSelect> SpawnParam;
    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FNameSelect> AutoCallFunction;
    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FNameSelect> ExecFunction;
    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FNameSelect> InDelegate;
    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FNameSelect> OutDelegate;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    bool bOwnerContextPin = false;

//++CK
private:
    TMap<FName, TMap<FName, FString>> _PinMetadataMap;
//--CK

protected:
    //UK2Node interface
    virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const override;
    // - End of UK2Node interface

    class UFunction* GetFactoryFunction() const;
    //UEdGraphPin* FindParamPinChecked(FString ContextName, FName NativePinName, EEdGraphPinDirection Direction = EGPD_MAX) const;
    UEdGraphPin* FindParamPin(const FString& ContextName, FName NativePinName, EEdGraphPinDirection Direction = EGPD_MAX) const;

    void ResetPinByNames(TSet<FName>& NameArray);
    void ResetPinByNames(TArray<FNameSelect>& NameArray);
    void RefreshNames(TArray<FNameSelect>& NameArray, bool bRemoveNone = true) const;

    void RemoveNodePin(FName PinName);
    void AddAutoCallFunction(FName PinName);
    void AddInputExec(FName PinName);
    void AddOutputDelegate(FName PinName);
    void AddInputDelegate(FName PinName);
    void AddSpawnParam(FName PinName);

//++CK
    virtual auto
    CustomizeProxyFactoryFunction(
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UK2Node_CallFunction* InCreateProxyObjecNode) -> void {};

    virtual auto
    Get_PinsHiddenByDefault() -> TSet<FName> { return {}; };
//--CK

    virtual void CollectSpawnParam(UClass* InClass, const bool bFullRefresh);

    virtual void GenerateFactoryFunctionPins(UClass* InClass);
    virtual void GenerateSpawnParamPins(UClass* InClass);

    virtual void GenerateAutoCallFunctionPins(UClass* InClass);
    virtual void GenerateExecFunctionPins(UClass* InClass);
    virtual void GenerateInputDelegatePins(UClass* InClass);
    virtual void GenerateOutputDelegatePins(UClass* InClass);

    virtual void CreatePinsForClass(UClass* InClass, bool bFullRefresh = true);
    virtual void CreatePinsForClassFunction(UClass* InClass, FName FnName, bool bRetValPins = true);

    bool ConnectSpawnProperties(
        const UClass* ClassToSpawn,
        const UEdGraphSchema_K2* Schema,
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UEdGraphPin*& LastThenPin,
        UEdGraphPin* SpawnedActorReturnPin);

private:
    void InvalidatePinTooltips() const { bPinTooltipsValid = false; }
    void GeneratePinTooltip(UEdGraphPin& Pin) const;
    mutable bool bPinTooltipsValid;

protected:
    struct FNodeHelper
    {
        struct FOutputPinAndLocalVariable
        {
            UEdGraphPin* OutputPin;
            class UK2Node_TemporaryVariable* TempVar;
            FOutputPinAndLocalVariable(UEdGraphPin* Pin, class UK2Node_TemporaryVariable* Var) : OutputPin(Pin), TempVar(Var) {}
            bool operator==(const UEdGraphPin* Pin) const { return Pin == OutputPin; }
        };

        static bool ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction);
        static bool CreateDelegateForNewFunction(
            UEdGraphPin* DelegateInputPin,
            FName FunctionName,
            UK2Node* CurrentNode,
            UEdGraph* SourceGraph,
            FKismetCompilerContext& CompilerContext);
        static bool CopyEventSignature(class UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema);
        static bool HandleDelegateImplementation(
            FMulticastDelegateProperty* CurrentProperty,
            const TArray<FOutputPinAndLocalVariable>& VariableOutputs,
            UEdGraphPin* ProxyObjectPin,
            UEdGraphPin*& InOutLastThenPin,
            UK2Node* CurrentNode,
            UEdGraph* SourceGraph,
            FKismetCompilerContext& CompilerContext);
    };

    struct FNames_Helper
    {
        static FString InPrefix;
        static FString OutPrefix;
        static FString InitPrefix;

        static FString CompiledFromBlueprintSuffix;

        static FString SkelPrefix;
        static FString ReinstPrefix;
        static FString DeadclassPrefix;
    };

    /*
    // Pin Redirector support
    struct FNodeHelperPinRedirectMapInfo
    {
        TMap<FName, TArray<UClass*> > OldPinToProxyClassMap;
    };
    static TMap<FName, FNodeHelperPinRedirectMapInfo> AsyncTaskPinRedirectMap;
    static bool bAsyncTaskPinRedirectMapInitialized;
    */

    /*
     //meta GetOptions dont work with TArray<FName>
    UFUNCTION()
    TArray<FString> FuncName() const
    {
        TArray<FString> Out;
        for (FName It : AllFunctions)
        {
            Out.Add(It.ToString());
        }
        return Out;
    }

    UPROPERTY(EditAnywhere, Category = "ExposeOptions", meta = (GetOptions = "FuncName"))
    FName Test;
    */
};
