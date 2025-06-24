#include "NodeCustomizations/BtfNameSelectStructCustomization.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Engine/GameViewportClient.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Commands/UIAction.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "EditorStyleSet.h"

void FBtf_NameSelectStructCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{

	PropertyHandle = StructPropertyHandle;
	GT = FromProperty();

	const auto& GetComboButtonText = [this]() -> FText { return FText::FromName(GT.Name); };
	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0.0f)
		.MinDesiredWidth(512.f)
		[
			SAssignNew(ComboButton, SComboButton)
			.ContentPadding(FMargin(4.0, 2.0))
			.ButtonContent()
			[
				SNew(STextBlock)
				.Font(IPropertyTypeCustomizationUtils::GetBoldFont())
				.Text_Lambda(GetComboButtonText)
			]
			.OnGetMenuContent_Lambda(
				[this]()
				{
					FMenuBuilder MenuBuilder(false, nullptr);
					{
						const FUIAction ItemAction(FExecuteAction::CreateRaw(this, &FBtf_NameSelectStructCustomization::OnValueCommitted, FName(NAME_None)));
						MenuBuilder.AddMenuEntry(
							FText::FromName(NAME_None),
							FText(),
							FSlateIcon(),
							ItemAction,
							NAME_None,
							EUserInterfaceActionType::Button);
					}

					if (GT.All)
					{
						for (FName It : *GT.All)
						{
							if (GT.Exclude == nullptr || !GT.Exclude->Contains(It))
							{
								FUIAction ItemAction(FExecuteAction::CreateRaw(this, &FBtf_NameSelectStructCustomization::OnValueCommitted, It));
								MenuBuilder.AddMenuEntry(
									FText::FromName(It),
									FText(),
									FSlateIcon(),
									ItemAction,
									NAME_None,
									EUserInterfaceActionType::Button);
							}
						}
					}
					return MenuBuilder.MakeWidget();
				})

		];
}

void FBtf_NameSelectStructCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	/* do nothing */
}

void FBtf_NameSelectStructCustomization::OnValueChanged(FName Val)
{
	GT.Name = Val;
}

void FBtf_NameSelectStructCustomization::OnValueCommitted(FName Val)
{
	if (PropertyHandle.IsValid())
	{
		GT.Name = Val;
		ComboButton->SetIsOpen(false, false);
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		for (const auto RawDataInstance : RawData)
		{
			*static_cast<FBtf_NameSelect*>(RawDataInstance) = GT;
		}

		PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		PropertyHandle->NotifyFinishedChangingProperties();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("PropertyHandle.IsValid() FAILED!"));
	}
}

FBtf_NameSelect FBtf_NameSelectStructCustomization::FromProperty() const
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	if (RawData.Num() != 1)
	{
		return FBtf_NameSelect();
	}
	const FBtf_NameSelect* DataPtr = static_cast<const FBtf_NameSelect*>(RawData[0]);
	if (DataPtr == nullptr)
	{
		return FBtf_NameSelect();
	}
	return *DataPtr;
}
