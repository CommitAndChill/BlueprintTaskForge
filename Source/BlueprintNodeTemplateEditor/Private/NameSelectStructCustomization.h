//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NameSelect.h"
#include "IPropertyTypeCustomization.h"

class IPropertyHandle;

/** */
class FNameSelectStructCustomization : public IPropertyTypeCustomization //, public FEditorUndoClient
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FNameSelectStructCustomization()); }

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

	FNameSelect GT;
	FNameSelect FromProperty() const;

private:
	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	TSharedPtr<class SComboButton> ComboButton;

public:
	FNameSelectStructCustomization();
};
