#pragma once

#include "CoreMinimal.h"
#include "BTF_NameSelect.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

/** */
class FBTF_NameSelectStructCustomization : public IPropertyTypeCustomization //, public FEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FBTF_NameSelectStructCustomization()); }

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

	FBTF_NameSelect GT;
	FBTF_NameSelect FromProperty() const;

private:
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<class SComboButton> ComboButton;

public:
	FBTF_NameSelectStructCustomization();
};
