// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#pragma once

#include "CoreMinimal.h"
#include "BtfNameSelect.h"
#include "IPropertyTypeCustomization.h"

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------
