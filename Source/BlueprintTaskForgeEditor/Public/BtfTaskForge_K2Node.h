#pragma once

#include "CoreMinimal.h"

#include "BtfExtendConstructObject_K2Node.h"

#include "BtfTaskForge_K2Node.generated.h"

/** */
UCLASS()
class BLUEPRINTTASKFORGEEDITOR_API UBtf_TaskForge_K2Node : public UBtf_ExtendConstructObject_K2Node
{
    GENERATED_BODY()
public:
    UBtf_TaskForge_K2Node(const FObjectInitializer& ObjectInitializer);

    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override
    {
        //++V
        /**Note: Without this check, the @ReconstructNode would somehow
         * lead to a chain of events that would mark the parent blueprint
         * as dirty as the editor is loading, leading to some blueprints
         * always being labelled as dirty. This check prevents that. */
        if(Decorator.IsValid())
        {
            /**If the node is using a decorator, we want to refresh the node in case the decorator
             * is reading the data from the input pins to drive its visuals.
             * If we instantly reconstruct the node, then we encounter a race condition when we drag
             * off the pin and create a variable and cause a crash. Using this timer gets around that
             * that race condition. */
            FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
            {
                ReconstructNode();
                return false;
            }));
        }
        //--V
    }
    virtual void AllocateDefaultPins() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetMenuCategory() const override;

//++CK

    auto Get_NodeDescription() const -> FString;
    auto Get_StatusString() const -> FString;
    auto Get_StatusBackgroundColor() const -> FLinearColor;
    auto Get_NodeConfigText() const -> FText;

    auto
    Get_PinsHiddenByDefault() -> TSet<FName>;
//--CK

#if WITH_EDITORONLY_DATA
    virtual void ResetToDefaultExposeOptions_Impl() override;
#endif

protected:
    void HideClassPin() const;
    void RegisterBlueprintAction(UClass* TargetClass, FBlueprintActionDatabaseRegistrar& ActionRegistrar) const;

    virtual void CollectSpawnParam(UClass* InClass, const bool bFullRefresh) override;
};
