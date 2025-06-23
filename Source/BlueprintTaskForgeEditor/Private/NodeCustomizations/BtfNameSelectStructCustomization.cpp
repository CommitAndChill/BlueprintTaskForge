#include "NodeCustomizations/BtfNameSelectStructCustomization.h"
#include "BftMacros.h"
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

// --------------------------------------------------------------------------------------------------------------------

FBtf_NameSelectStructCustomization::FBtf_NameSelectStructCustomization()
{
}

TSharedRef<IPropertyTypeCustomization> FBtf_NameSelectStructCustomization::MakeInstance()
{
    return MakeShareable(new FBtf_NameSelectStructCustomization());
}

void FBtf_NameSelectStructCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    PropertyHandle = StructPropertyHandle;
    GT = FromProperty();

    const auto GetComboButtonText = [this]() -> FText { return FText::FromName(GT.Get_Name()); };
    
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
                    auto MenuBuilder = FMenuBuilder(false, nullptr);
                    {
                        const auto ItemAction = FUIAction(FExecuteAction::CreateRaw(this, &FBtf_NameSelectStructCustomization::OnValueCommitted, FName(NAME_None)));
                        MenuBuilder.AddMenuEntry(
                            FText::FromName(FName(NAME_None)),
                            FText(),
                            FSlateIcon(),
                            ItemAction,
                            NAME_None,
                            EUserInterfaceActionType::Button);
                    }

                    if (NOT GT.Get_All())
                    { return MenuBuilder.MakeWidget(); }

                    for (const auto& It : *GT.Get_All())
                    {
                        if (GT.Get_Exclude() != nullptr && GT.Get_Exclude()->Contains(It))
                        { continue; }

                        const auto ItemAction = FUIAction(FExecuteAction::CreateRaw(this, &FBtf_NameSelectStructCustomization::OnValueCommitted, It));
                        MenuBuilder.AddMenuEntry(
                            FText::FromName(It),
                            FText(),
                            FSlateIcon(),
                            ItemAction,
                            NAME_None,
                            EUserInterfaceActionType::Button);
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
}

void FBtf_NameSelectStructCustomization::OnValueChanged(FName Val)
{
    // Note: This would need to set the value through a proper mechanism
    // Since Name is now private, we'd need a setter method in FBtf_NameSelect
}

void FBtf_NameSelectStructCustomization::OnValueCommitted(FName Val)
{
    if (NOT PropertyHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("PropertyHandle.IsValid() FAILED!"));
        return;
    }

    // Create a new instance with the new value
    auto NewGT = FBtf_NameSelect(Val);
#if WITH_EDITORONLY_DATA
    // Copy the editor-only data
    if (GT.Get_All())
    {
        NewGT.SetAllExclude(*GT.Get_All(), GT.Get_Exclude() ? *GT.Get_Exclude() : TArray<FBtf_NameSelect>{});
    }
#endif
    GT = NewGT;
    
    ComboButton->SetIsOpen(false, false);
    auto RawData = TArray<void*>{};
    PropertyHandle->AccessRawData(RawData);

    for (const auto& RawDataInstance : RawData)
    {
        *static_cast<FBtf_NameSelect*>(RawDataInstance) = GT;
    }

    PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
    PropertyHandle->NotifyFinishedChangingProperties();
}

FBtf_NameSelect FBtf_NameSelectStructCustomization::FromProperty() const
{
    auto RawData = TArray<void*>{};
    PropertyHandle->AccessRawData(RawData);

    if (RawData.Num() != 1)
    { return FBtf_NameSelect(); }
    
    const auto* DataPtr = static_cast<const FBtf_NameSelect*>(RawData[0]);
    if (NOT DataPtr)
    { return FBtf_NameSelect(); }
    
    return *DataPtr;
}

// --------------------------------------------------------------------------------------------------------------------
