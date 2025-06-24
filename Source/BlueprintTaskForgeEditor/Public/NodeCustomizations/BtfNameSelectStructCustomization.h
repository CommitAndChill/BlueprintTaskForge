#pragma once

#include "CoreMinimal.h"
#include "BtfNameSelect.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

/** */
class FBtf_NameSelectStructCustomization : public IPropertyTypeCustomization //, public FEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FBtf_NameSelectStructCustomization()); }

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

	FBtf_NameSelect GT;
	FBtf_NameSelect FromProperty() const;

private:
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<class SComboButton> ComboButton;
};
