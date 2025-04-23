//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#include "NameSelectStructCustomization.h"

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


void FNameSelectStructCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{

	PropertyHandle = StructPropertyHandle;
	GT = FromProperty();

	const auto& GetComboButtonText = [this]() -> FText { return FText::FromName(GT.Name); };
	// clang-format off
	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0.0f)
		.MinDesiredWidth(512.f)
		[
			SAssignNew(ComboButton, SComboButton)
			//.ComboButtonStyle(FButtonStyle())
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
						const FUIAction ItemAction(FExecuteAction::CreateRaw(this, &FNameSelectStructCustomization::OnValueCommitted, FName(NAME_None)));
						MenuBuilder.AddMenuEntry(
							FText::FromName(FName(NAME_None)),
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
							
								FUIAction ItemAction(FExecuteAction::CreateRaw(this, &FNameSelectStructCustomization::OnValueCommitted, It)/*, FCanExecuteAction(), FIsActionChecked()*/); //,  FIsActionChecked::CreateLambda([this]() -> bool { return true; })
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
	// clang-format on
}

void FNameSelectStructCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	/* do nothing */
}

void FNameSelectStructCustomization::OnValueChanged(FName Val)
{
	GT.Name = Val;
}

void FNameSelectStructCustomization::OnValueCommitted(FName Val)
{
	if (PropertyHandle.IsValid())
	{
		GT.Name = Val;
		ComboButton->SetIsOpen(false, false);
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);
		//PropertyHandle->NotifyPreChange();

		for (const auto RawDataInstance : RawData)
		{
			*static_cast<FNameSelect*>(RawDataInstance) = GT;
		}

		PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		PropertyHandle->NotifyFinishedChangingProperties();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT(" PropertyHandle.IsValid() FAILED!"));
	}
}

FNameSelect FNameSelectStructCustomization::FromProperty() const
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);

	if (RawData.Num() != 1)
	{
		return FNameSelect();
	}
	const FNameSelect* DataPtr = static_cast<const FNameSelect*>(RawData[0]);
	if (DataPtr == nullptr)
	{
		return FNameSelect();
	}
	return *DataPtr;
}

FNameSelectStructCustomization::FNameSelectStructCustomization()
{}
