#pragma once

#include "CoreMinimal.h"
#include "BtfNameSelect.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

// --------------------------------------------------------------------------------------------------------------------

class FBtf_NameSelectStructCustomization : public IPropertyTypeCustomization //, public FEditorUndoClient
{
public:
	FBtf_NameSelectStructCustomization();

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
		
	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void OnValueChanged(FName Val);
	void OnValueCommitted(FName Val);

	FBtf_NameSelect FromProperty() const;

public:
	FBtf_NameSelect GT;

private:
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<class SComboButton> ComboButton;
};

// --------------------------------------------------------------------------------------------------------------------
