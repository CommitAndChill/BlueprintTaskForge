#include "NodeCustomizations/Btf_NodeDetailsCustomizations.h"

#include "Btf_ExtendConstructObject_K2Node.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "Btf_TaskForge.h"

#include "NodeDecorators/Btf_NodeDecorator.h"

TSharedRef<IDetailCustomization> FBtf_NodeDetailsCustomizations::MakeInstance()
{
	return MakeShareable(new FBtf_NodeDetailsCustomizations);
}

void FBtf_NodeDetailsCustomizations::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

	if(CustomizedObjects.Num() != 1)
	{
		return;
	}

	TArray<FName> CategoryNames;
	DetailBuilder.GetCategoryNames(CategoryNames);

	/**The category we're about to create will annoyingly go to the top of the
	 * details panel. Update all categories to 0, then we'll update the other
	 * details panel to be 1 to force it to go to the bottom. */
	for(auto& CurrentCategory : CategoryNames)
	{
		IDetailCategoryBuilder& SetupCategory = DetailBuilder.EditCategory(CurrentCategory);
		SetupCategory.SetSortOrder(0);
	}

	auto* TaskNode = Cast<UBtf_ExtendConstructObject_K2Node>(CustomizedObjects[0].Get());

	TaskNode->DetailsPanelBuilder = &DetailBuilder;

	if(TaskNode && TaskNode->ProxyClass && TaskNode->AllowInstance && TaskNode->TaskInstance)
	{
		//Set the filter function to exclude non-instance-editable properties before we add
		//the task instance to the details panel
		DetailBuilder.GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
			[=](const FPropertyAndParent& PropertyAndParent) -> bool
		{
			//Check if the property is marked as instance-editable and public. If not, hide them
			const FProperty* Property = &PropertyAndParent.Property;
			const bool bIsPublic = Property->HasAnyPropertyFlags(CPF_Edit | CPF_EditConst);
				const bool bIsInstanceEditable = !Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);

			/**Properties inside of structs aren't instance editable, even though the
			 * struct itself may be. We need to check if the property is a child of a struct */
			bool IsChildOfStruct = PropertyAndParent.ParentProperties.IsValidIndex(0);

			return (bIsPublic && bIsInstanceEditable) || IsChildOfStruct;
		}));

		/**Create a custom category and add the details panel to it
		 * to create some separation between the nodes properties and the task object properties.
		 *
		 * We also set the sort order to 1 to ensure that the category is displayed below the default category.*/
		IDetailCategoryBuilder& SecondaryCategory = DetailBuilder.EditCategory("Task Object");
		SecondaryCategory.InitiallyCollapsed(false);
		SecondaryCategory.SetSortOrder(1);
		FAddPropertyParams AddPropertyParams;
		AddPropertyParams.CreateCategoryNodes(true);
		AddPropertyParams.HideRootObjectNode(true);
		/**Add the task instance as an external object, so we can edit its properties*/
		SecondaryCategory.AddExternalObjects({TaskNode->TaskInstance}, EPropertyLocation::Default, AddPropertyParams);
	}

	if(TaskNode->Decorator.IsValid())
	{
		/**Start adding any objects the decorator wants to include in the details panel*/
		for(auto& CurrentObject : TaskNode->Decorator.Get()->GetObjectsForExtraDetailsPanels())
		{
			//Create a new details view for the current object
			FDetailsViewArgs CurrentObjectDetailsViewArgs;
			CurrentObjectDetailsViewArgs.bHideSelectionTip = true;
			CurrentObjectDetailsViewArgs.bAllowSearch = true;

			DetailBuilder.GetDetailsView()->OnFinishedChangingProperties().AddLambda(
				[TaskNode](const FPropertyChangedEvent& PropertyChangedEvent)
			{
				//Refresh the node, in-case the decorator is displaying info from the object
				if(TaskNode)
				{
					TaskNode->ReconstructNode();
				}
			});

			/**Reset the visibility delegate to allow everything, in case the TaskInstance
			 * delegate section was triggered, which only allows instance editable
			 * properties to appear. */
			DetailBuilder.GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
				[=](const FPropertyAndParent& PropertyAndParent) -> bool
			{
				return true;
			}));

			/**Create a custom category and add the details panel to it
			 * to create some separation between the nodes properties and the task object properties.
			 *
			 * We also set the sort order to 2 to ensure that the category is displayed below the task instance. */
			IDetailCategoryBuilder& CurrentObjectCategory = DetailBuilder.EditCategory(
				FName(FString::Printf(TEXT("%s"), *CurrentObject->GetName())));
			CurrentObjectCategory.SetSortOrder(2);
			FAddPropertyParams AddPropertyParams;
			AddPropertyParams.CreateCategoryNodes(true);
			AddPropertyParams.HideRootObjectNode(true);
			CurrentObjectCategory.AddExternalObjects({CurrentObject}, EPropertyLocation::Default, AddPropertyParams);
		}
	}
}
