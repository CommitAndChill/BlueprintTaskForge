#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Engine/MemberReference.h"
#include "UObject/ObjectMacros.h"
#include "BtfNameSelect.h"

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

UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_ExtendConstructObject_K2Node : public UK2Node
{
    GENERATED_BODY()
public:
    UBtf_ExtendConstructObject_K2Node(const FObjectInitializer& ObjectInitializer);

	// - UEdGraphNode interface			// UK2Node_BaseAsyncTask
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

	/**Not the same as @ValidateNodeDuringCompilation.
	 * This is called at the start of @ExpandNode so most
	 * variables that we need are valid for the context of
	 * what this node does. */
	virtual bool CanBePlacedInGraph() const;
	virtual void GenerateCustomOutputPins();
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	FName GetTemplateInstanceName() const { return FName(ProxyClass->GetName() + NodeGuid.ToString()); }

	virtual void DestroyNode() override;

    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    //virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const override;
    // - End of UEdGraphNode interface

	/**Used for scenarios where we need to run a function or retrieve
	 * a value from the CDO, but if we are using a instance, we will
	 * return the instance instead. */
	UBtf_TaskForge* GetInstanceOrDefaultObject() const;

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

	UPROPERTY()
	FName WorldContextPinName = FName(TEXT("Outer"));
	UPROPERTY()
	FName ClassPinName = FName(TEXT("Class"));
	UPROPERTY()
	FName OutPutObjectPinName = FName(TEXT("Object"));
	UPROPERTY()
	FName NodeGuidStrName = FName(TEXT("NodeGuidStr"));

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
    bool bOwnerContextPin = false;

	UPROPERTY(EditAnywhere, Category = "Instance")
	bool AllowInstance = false;

	UPROPERTY()
	UBtf_TaskForge* TaskInstance = nullptr;

	/**V: For some extra details panel customization, I'm using the
	 * decorator as a proxy class to simplify the API.
	 * But for some reason, there's no "GetNodeWidget" function and
	 * the "NodeWidget" property is deprecated. So I'm storing an
	 * extra reference in here until Epic provide a true way
	 * of retrieving the node widget. */
    UPROPERTY()
	TWeakObjectPtr<UBtf_NodeDecorator> Decorator = nullptr;

	UPROPERTY()
	TArray<FCustomOutputPin> CustomPins;

	/**For some reason, the typical @SetOnPropertyValueChanged delegate
	 * is not working for this node. So we store a pointer to the details
	 * panel and when the @AllowInstance property is changed,
	 * we manually refresh the details panel. */
	IDetailLayoutBuilder* DetailsPanelBuilder = nullptr;

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
    void ResetPinByNames(TArray<FBtf_NameSelect>& NameArray);
    void RefreshNames(TArray<FBtf_NameSelect>& NameArray, bool bRemoveNone = true) const;

    void RemoveNodePin(FName PinName);
    void AddAutoCallFunction(FName PinName);
    void AddInputExec(FName PinName);
    void AddOutputDelegate(FName PinName);
    void AddInputDelegate(FName PinName);
    void AddSpawnParam(FName PinName);

//++CK
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
        	FOutputPinAndLocalVariable() = default;
            UEdGraphPin* OutputPin;
            class UK2Node_TemporaryVariable* TempVar;
            FOutputPinAndLocalVariable(UEdGraphPin* Pin, class UK2Node_TemporaryVariable* Var) : OutputPin(Pin), TempVar(Var) {}
            bool operator==(const UEdGraphPin* Pin) const { return Pin == OutputPin; }
        };

        static bool ValidDataPin(
            const UEdGraphPin* Pin,
            EEdGraphPinDirection Direction);

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
