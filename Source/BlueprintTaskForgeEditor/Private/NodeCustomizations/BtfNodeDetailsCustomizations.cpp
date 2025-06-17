#include "NodeCustomizations/BtfNodeDetailsCustomizations.h"
#include "BftMacros.h"

#include "BtfExtendConstructObject_K2Node.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "BtfTaskForge.h"

#include "NodeDecorators/BtfNodeDecorator.h"

TSharedRef<IDetailCustomization> FBtf_NodeDetailsCustomizations::MakeInstance()
{
    return MakeShareable(new FBtf_NodeDetailsCustomizations);
}

void FBtf_NodeDetailsCustomizations::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
    DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

    if (CustomizedObjects.Num() != 1)
    {
        return;
    }

    TArray<FName> CategoryNames;
    DetailBuilder.GetCategoryNames(CategoryNames);

    for (auto& CurrentCategory : CategoryNames)
    {
        auto& SetupCategory = DetailBuilder.EditCategory(CurrentCategory);
        SetupCategory.SetSortOrder(0);
    }

    auto* TaskNode = Cast<UBtf_ExtendConstructObject_K2Node>(CustomizedObjects[0].Get());

    if (NOT IsValid(TaskNode))
    {
        return;
    }

    TaskNode->DetailsPanelBuilder = &DetailBuilder;

    if (TaskNode && TaskNode->ProxyClass && TaskNode->AllowInstance && TaskNode->TaskInstance)
    {
        DetailBuilder.GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
            [=](const FPropertyAndParent& PropertyAndParent) -> bool
        {
            const auto& Property = &PropertyAndParent.Property;
            const auto& IsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
            const auto& IsInstanceEditable = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

            const auto& IsChildOfStruct = PropertyAndParent.ParentProperties.IsValidIndex(0);

            return (IsPublic && IsInstanceEditable) || IsChildOfStruct;
        }));

        auto& SecondaryCategory = DetailBuilder.EditCategory("Task Object");
        SecondaryCategory.InitiallyCollapsed(false);
        SecondaryCategory.SetSortOrder(1);
        FAddPropertyParams AddPropertyParams;
        AddPropertyParams.CreateCategoryNodes(true);
        AddPropertyParams.HideRootObjectNode(true);
        SecondaryCategory.AddExternalObjects({TaskNode->TaskInstance}, EPropertyLocation::Default, AddPropertyParams);
    }

    if (TaskNode->Decorator.IsValid())
    {
        for (auto& CurrentObject : TaskNode->Decorator.Get()->GetObjectsForExtraDetailsPanels())
        {
            FDetailsViewArgs CurrentObjectDetailsViewArgs;
            CurrentObjectDetailsViewArgs.bHideSelectionTip = true;
            CurrentObjectDetailsViewArgs.bAllowSearch = true;

            DetailBuilder.GetDetailsView()->OnFinishedChangingProperties().AddLambda(
                [TaskNode](const FPropertyChangedEvent& PropertyChangedEvent)
            {
                if (TaskNode)
                {
                    TaskNode->ReconstructNode();
                }
            });

            DetailBuilder.GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
                [=](const FPropertyAndParent& PropertyAndParent) -> bool
            {
                return true;
            }));

            auto& CurrentObjectCategory = DetailBuilder.EditCategory(
                FName(FString::Printf(TEXT("%s"), *CurrentObject->GetName())));
            CurrentObjectCategory.SetSortOrder(2);
            FAddPropertyParams AddPropertyParams;
            AddPropertyParams.CreateCategoryNodes(true);
            AddPropertyParams.HideRootObjectNode(true);
            CurrentObjectCategory.AddExternalObjects({CurrentObject}, EPropertyLocation::Default, AddPropertyParams);
        }
    }
}