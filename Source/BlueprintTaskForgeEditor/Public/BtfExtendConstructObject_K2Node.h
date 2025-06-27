#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "UObject/ObjectMacros.h"
#include "BtfNameSelect.h"
#include "BftMacros.h"

#include "K2Node_DynamicCast.h"

#include "BtfExtendConstructObject_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UBtf_NodeDecorator;
class UBtf_TaskForge;
struct FCustomOutputPin;
class FBlueprintActionDatabaseRegistrar;
class UEdGraph;
class UEdGraphPin;
class UEdGraphSchema;
class UEdGraphSchema_K2;

UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_ExtendConstructObject_K2Node : public UK2Node
{
    GENERATED_BODY()

public:
    UBtf_ExtendConstructObject_K2Node(const FObjectInitializer& ObjectInitializer);

    // UEdGraphNode Interface
    virtual void AllocateDefaultPins() override;
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
    virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
    virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
    virtual bool ShouldShowNodeProperties() const override { return true; }
    virtual bool CanDuplicateNode() const override { return true; }
    virtual void PostPasteNode() override;
    virtual UObject* GetJumpTargetForDoubleClick() const override;
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const override;
    virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override { return false; }
    virtual void ReconstructNode() override;
    virtual void PostReconstructNode() override;
    virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
    virtual void DestroyNode() override;
    virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

    // UK2Node Interface
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;
    virtual FName GetCornerIcon() const override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    virtual bool UseWorldContext() const;
    virtual FString GetPinMetaData(FName PinName, FName Key) override;

    virtual void Serialize(FArchive& Ar) override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "ExposeOptions")
    void ResetToDefaultExposeOptions() { ResetToDefaultExposeOptions_Impl(); }
    virtual void ResetToDefaultExposeOptions_Impl();
#endif

    // Custom Functions
    UEdGraphPin* GetClassPin() const { return FindPin(ClassPinName); }
    UEdGraphPin* GetWorldContextPin() const;
    bool CanBePlacedInGraph() const;
    void GenerateCustomOutputPins();
    FName GetTemplateInstanceName() const { return FName(ProxyClass->GetName() + NodeGuid.ToString()); }
    UBtf_TaskForge* GetInstanceOrDefaultObject() const;

    // Pin Names
    UPROPERTY()
    FName WorldContextPinName = FName(TEXT("Outer"));

    UPROPERTY()
    FName ClassPinName = FName(TEXT("Class"));

    UPROPERTY()
    FName OutPutObjectPinName = FName(TEXT("Object"));

    UPROPERTY()
    FName NodeGuidStrName = FName(TEXT("NodeGuidStr"));

    // Core Properties
    UPROPERTY()
    UClass* ProxyFactoryClass;

    UPROPERTY()
    UClass* ProxyClass;

    UPROPERTY()
    FName ProxyFactoryFunctionName;

    // Configuration Properties
    UPROPERTY()
    TSet<FName> AllDelegates;

    UPROPERTY()
    TSet<FName> AllFunctions;

    UPROPERTY()
    TSet<FName> AllFunctionsExec;

    UPROPERTY()
    TSet<FName> AllParam;

    UPROPERTY()
    bool SelfContext = false;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FBtf_NameSelect> SpawnParam;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FBtf_NameSelect> AutoCallFunction;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FBtf_NameSelect> ExecFunction;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FBtf_NameSelect> InDelegate;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    TArray<FBtf_NameSelect> OutDelegate;

    UPROPERTY(EditAnywhere, Category = "ExposeOptions")
    bool OwnerContextPin = false;

    UPROPERTY(EditAnywhere, Category = "Instance")
    bool AllowInstance = false;

    UPROPERTY()
    UBtf_TaskForge* TaskInstance = nullptr;

    UPROPERTY()
    TWeakObjectPtr<UBtf_NodeDecorator> Decorator = nullptr;

    UPROPERTY()
    TArray<FCustomOutputPin> CustomPins;

    IDetailLayoutBuilder* DetailsPanelBuilder = nullptr;

protected:
    // UK2Node Interface
    virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const override;

    // Helper Functions
    class UFunction* GetFactoryFunction() const;
    UEdGraphPin* FindParamPin(const FString& ContextName, FName NativePinName, EEdGraphPinDirection Direction = EGPD_MAX) const;
    void ResetPinByNames(TSet<FName>& NameArray);
    void ResetPinByNames(TArray<FBtf_NameSelect>& NameArray);
    void RefreshNames(TArray<FBtf_NameSelect>& NameArray, bool RemoveNone = true) const;

    // Pin Management
    void RemoveNodePin(FName PinName);
    void AddAutoCallFunction(FName PinName);
    void AddInputExec(FName PinName);
    void AddOutputDelegate(FName PinName);
    void AddInputDelegate(FName PinName);
    void AddSpawnParam(FName PinName);

    virtual TSet<FName> Get_PinsHiddenByDefault() { return {}; };
    virtual void CollectSpawnParam(UClass* TargetClass, const bool FullRefresh);

    // Pin Generation
    virtual void GenerateFactoryFunctionPins(UClass* TargetClass);
    virtual void GenerateSpawnParamPins(UClass* TargetClass);
    virtual void GenerateAutoCallFunctionPins(UClass* TargetClass);
    virtual void GenerateExecFunctionPins(UClass* TargetClass);
    virtual void GenerateInputDelegatePins(UClass* TargetClass);
    virtual void GenerateOutputDelegatePins(UClass* TargetClass);
    virtual void CreatePinsForClass(UClass* TargetClass, bool FullRefresh = true);
    virtual void CreatePinsForClassFunction(UClass* TargetClass, FName FunctionName, bool RetValPins = true);

    bool ConnectSpawnProperties(
        const UClass* ClassToSpawn,
        const UEdGraphSchema_K2* Schema,
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UEdGraphPin*& LastThenPin,
        UEdGraphPin* SpawnedActorReturnPin);

private:
    void InvalidatePinTooltips() const { PinTooltipsValid = false; }
    void GeneratePinTooltip(UEdGraphPin& Pin) const;
    mutable bool PinTooltipsValid;

    TMap<FName, TMap<FName, FString>> PinMetadataMap;

#if WITH_EDITORONLY_DATA
    // Helper methods for PostEditChangeProperty
    void HandleAutoCallFunctionChange();
    void HandleExecFunctionChange();
    void HandleInDelegateChange();
    void HandleOutDelegateChange();
    void HandleSpawnParamChange();
    void HandleAllowInstanceChange();
    void CreateOrFindTaskInstance();
    void DestroyTaskInstance();
    void BindTaskInstancePropertyChanged();
#endif

    // Helper methods for Serialize
    void ValidateProxyClass();
    void HandleLegacyWorldContextPin();

    // Helper methods for GetNodeContextMenuActions
    bool IsValidContextForMenu(const UGraphNodeContextMenuContext* Context) const;
    void AddNodeContextMenuActions(FToolMenuSection& Section) const;
    void AddSubmenuForItems(
        FToolMenuSection& Section,
        const FName& SubmenuName,
        const FString& SubmenuLabel,
        const TSet<FName>& AllItems,
        const TArray<FBtf_NameSelect>& CurrentItems,
        void (UBtf_ExtendConstructObject_K2Node::*AddFunction)(const FName) const) const;
    void AddPinContextMenuActions(FToolMenuSection& Section, const UEdGraphPin* Pin) const;
    bool IsSystemPin(const UEdGraphPin* Pin) const;
    bool IsRemovablePin(const UEdGraphPin* Pin, const FName& PinName) const;

    // Helper methods for ValidateNodeDuringCompilation
    void ValidateMacroUsage(FCompilerResultsLog& MessageLog) const;
    bool IsInUbergraph() const;
    void ValidateProxyClass(FCompilerResultsLog& MessageLog) const;
    void ValidateClassPin(FCompilerResultsLog& MessageLog) const;
    void ValidateGraphPlacement(FCompilerResultsLog& MessageLog) const;
    void ValidateTaskInstance(FCompilerResultsLog& MessageLog) const;

    // Helper methods for GetPinHoverText
    void RefreshPinTooltips() const;

protected:
    struct FNodeHelper
    {
        struct FOutputPinAndLocalVariable
        {
            FOutputPinAndLocalVariable() = default;
            UEdGraphPin* OutputPin;
            class UK2Node_TemporaryVariable* TempVar;
            FOutputPinAndLocalVariable(UEdGraphPin* Pin, class UK2Node_TemporaryVariable* Var) : OutputPin(Pin), TempVar(Var) {}
            bool operator==(const UEdGraphPin* Pin) const { return Pin == OutputPin; }
        };

        static bool ValidDataPin(const UEdGraphPin* Pin, EEdGraphPinDirection Direction);
        static bool CreateDelegateForNewFunction(UEdGraphPin* DelegateInputPin, FName FunctionName, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);
        static bool CopyEventSignature(class UK2Node_CustomEvent* CENode, UFunction* Function, const UEdGraphSchema_K2* Schema);
        static bool HandleDelegateImplementation(FMulticastDelegateProperty* CurrentProperty, const TArray<FOutputPinAndLocalVariable>& VariableOutputs, UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin, UK2Node* CurrentNode, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);
        static bool HandleCustomPinsImplementation(FMulticastDelegateProperty* CurrentProperty, UEdGraphPin* ProxyObjectPin, UEdGraphPin*& InOutLastThenPin, UK2Node* CurrentNode, UEdGraph* SourceGraph, TArray<FCustomOutputPin> OutputNames, FKismetCompilerContext& CompilerContext);
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
};

// --------------------------------------------------------------------------------------------------------------------
