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

#include "KismetNodes/SGraphNodeK2Base.h"

#include "NodeDecorators/BtfNodeDecorator.h"

// --------------------------------------------------------------------------------------------------------------------

class UBtf_NodeDecorator;
class UBtf_TaskForge_K2Node;
class UBtf_TaskForge;

class SBtf_Node : public SGraphNodeK2Base
{
public:

	SLATE_BEGIN_ARGS(SBtf_Node) {}
	SLATE_END_ARGS()

	UClass* TaskClass = nullptr;

	TObjectPtr<UBtf_NodeDecorator> Decorator = nullptr;

	virtual ~SBtf_Node()
	{
		if(Decorator)
		{
			Decorator->RemoveFromRoot();
		}
	}

	void Construct(const FArguments& InArgs, UEdGraphNode* InGraphNode, UClass* InTaskClass);

	virtual TSharedRef<SWidget> CreateNodeContentArea() override;
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;

	auto CreateBelowPinControls(
		TSharedPtr<SVerticalBox> MainBox) -> void override;

	auto Get_NodeConfigTextVisibility() const -> EVisibility;
	auto Get_NodeConfigText() const -> FText;

protected:
	TWeakObjectPtr<UBtf_TaskForge_K2Node> _BlueprintTaskNode;
	TSharedPtr<STextBlock> _ConfigTextBlock;
};

// --------------------------------------------------------------------------------------------------------------------