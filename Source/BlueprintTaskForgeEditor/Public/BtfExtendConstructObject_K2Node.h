#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "UObject/ObjectMacros.h"
#include "BtfNameSelect.h"
#include "BftMacros.h"
#include "K2Node_DynamicCast.h"
#include "BtfExtendConstructObject_K2Node.generated.h"

class UBtf_NodeDecorator;
class UBtf_TaskForge;
struct FCustomOutputPin;
class FBlueprintActionDatabaseRegistrar;
class UEdGraph;
class UEdGraphPin;
class UEdGraphSchema;
class UEdGraphSchema_K2;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_ExtendConstructObject_K2Node : public UK2Node
{
    GENERATED_BODY()

public:
    UBtf_ExtendConstructObject_K2Node(const FObjectInitializer& ObjectInitializer);

    // Core Overrides
    virtual void AllocateDefaultPins() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void ReconstructNode() override;
    virtual void PostReconstructNode() override;
    virtual void PostPasteNode() override;
    virtual void DestroyNode() override;
    virtual void Serialize(FArchive& Ar) override;

    // Node Information
    virtual FText GetTooltipText() const override;
    virtual FLinearColor GetNodeTitleColor() const override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FName GetCornerIcon() const override;

    // Validation
    virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
    virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
    virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
    virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;

    // Menu and Actions
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual FText GetMenuCategory() const override;
    virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
    virtual void GetMenuEntries(struct FGraphContextMenuBuilder& ContextMenuBuilder) const override;

    // Pin Management
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;

    // Compilation
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

    // Dependencies
    virtual bool HasExternalDependencies(TArray<class UStruct*>* OptionalOutput) const override;

    // Node Properties
    virtual bool ShouldShowNodeProperties() const override;
    virtual bool CanDuplicateNode() const override;
    virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
    virtual UObject* GetJumpTargetForDoubleClick() const override;

    // Utility Functions
    virtual bool UseWorldContext() const;
    virtual FString GetPinMetaData(FName InPinName, FName InKey) override;
    virtual bool CanBePlacedInGraph() const;
    virtual void GenerateCustomOutputPins();
    virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
    FName GetTemplateInstanceName() const;

    // Task Management
    UBtf_TaskForge* GetInstanceOrDefaultObject() const;

    // Pin Accessors
    UEdGraphPin* GetClassPin() const;
    UEdGraphPin* GetWorldContextPin() const;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty(FPropertyChangedEvent& e) override;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "ExposeOptions")
    void ResetToDefaultExposeOptions();

protected:
    virtual void ResetToDefaultExposeOptions_Impl();
#endif

public:
    // Pin Names
    UPROPERTY()
    FName WorldContextPinName = FName(TEXT("Outer"));
    
    UPROPERTY()
    FName ClassPinName = FName(TEXT("Class"));
    
    UPROPERTY()
    FName OutPutObjectPinName = FName(TEXT("Object"));
    
    UPROPERTY()
    FName NodeGuidStrName = FName(TEXT("NodeGuidStr"));

    // Factory Configuration
    UPROPERTY()
    UClass* ProxyFactoryClass;
    
    UPROPERTY()
    UClass* ProxyClass;
    
    UPROPERTY()
    FName ProxyFactoryFunctionName;

    // Collected Properties
    UPROPERTY()
    TSet<FName> AllDelegates;
    
    UPROPERTY()
    TSet<FName> AllFunctions;
    
    UPROPERTY()
    TSet<FName> AllFunctionsExec;
    
    UPROPERTY()
    TSet<FName> AllParam;

    // Context Configuration
    UPROPERTY()
    bool SelfContext = false;

    // Expose Options
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

    // Instance Configuration
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
    virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const override;

    // Factory Functions
    class UFunction* GetFactoryFunction() const;
    UEdGraphPin* FindParamPin(const FString& ContextName, FName NativePinName, EEdGraphPinDirection Direction = EGPD_MAX) const;

    // Pin Management
    void ResetPinByNames(TSet<FName>& NameArray);
    void ResetPinByNames(TArray<FBtf_NameSelect>& NameArray);
    void RefreshNames(TArray<FBtf_NameSelect>& NameArray, bool RemoveNone = true) const;

    // Context Menu Actions
    void RemoveNodePin(FName PinName);
    void AddAutoCallFunction(FName PinName);
    void AddInputExec(FName PinName);
    void AddOutputDelegate(FName PinName);
    void AddInputDelegate(FName PinName);
    void AddSpawnParam(FName PinName);

    virtual TSet<FName> Get_PinsHiddenByDefault();
    virtual void CollectSpawnParam(UClass* InClass, const bool FullRefresh);

    // Pin Generation
    virtual void GenerateFactoryFunctionPins(UClass* InClass);
    virtual void GenerateSpawnParamPins(UClass* InClass);
    virtual void GenerateAutoCallFunctionPins(UClass* InClass);
    virtual void GenerateExecFunctionPins(UClass* InClass);
    virtual void GenerateInputDelegatePins(UClass* InClass);
    virtual void GenerateOutputDelegatePins(UClass* InClass);

    virtual void CreatePinsForClass(UClass* InClass, bool FullRefresh = true);
    virtual void CreatePinsForClassFunction(UClass* InClass, FName FnName, bool RetValPins = true);

    bool ConnectSpawnProperties(
        const UClass* ClassToSpawn,
        const UEdGraphSchema_K2* Schema,
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph,
        UEdGraphPin*& LastThenPin,
        UEdGraphPin* SpawnedActorReturnPin);

private:
    void InvalidatePinTooltips() const;
    void GeneratePinTooltip(UEdGraphPin& Pin) const;
    mutable bool PinTooltipsValid;

    TMap<FName, TMap<FName, FString>> PinMetadataMap;

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

        static bool CreateDelegateForNewFunction(
            UEdGraphPin* DelegateInputPin,
            FName FunctionName,
            UK2Node* CurrentNode,
            UEdGraph* SourceGraph,
            FKismetCompilerContext& CompilerContext);

        static bool CopyEventSignature(
            class UK2Node_CustomEvent* CENode,
            UFunction* Function,
            const UEdGraphSchema_K2* Schema);

        static bool HandleDelegateImplementation(
            FMulticastDelegateProperty* CurrentProperty,
            const TArray<FOutputPinAndLocalVariable>& VariableOutputs,
            UEdGraphPin* ProxyObjectPin,
            UEdGraphPin*& InOutLastThenPin,
            UK2Node* CurrentNode,
            UEdGraph* SourceGraph,
            FKismetCompilerContext& CompilerContext);

        static bool HandleCustomPinsImplementation(
            FMulticastDelegateProperty* CurrentProperty,
            const TArray<FOutputPinAndLocalVariable>& VariableOutputs,
            UEdGraphPin* ProxyObjectPin,
            UEdGraphPin*& InOutLastThenPin,
            UK2Node* CurrentNode,
            UEdGraph* SourceGraph,
            TArray<FCustomOutputPin> OutputNames,
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
};

// --------------------------------------------------------------------------------------------------------------------
