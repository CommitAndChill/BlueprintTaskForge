﻿// Copyright (c) 2025 BlueprintTaskForge Maintainers
// 
// This file is part of the BlueprintTaskForge Plugin for Unreal Engine.
// 
// Licensed under the BlueprintTaskForge Open Plugin License v1.0 (BTFPL-1.0).
// You may obtain a copy of the license at:
// https://github.com/CommitAndChill/BlueprintTaskForge/blob/main/LICENSE.md
// 
// SPDX-License-Identifier: BTFPL-1.0

#include "Tools/BtfSTaskPalette.h"

#include "Settings/BtfEditorSettings.h"

#include "BlueprintActionMenuBuilder.h"
#include "BlueprintEditor.h"
#include "BlueprintNodeSpawner.h"
#include "BtfTaskForge.h"
#include "BtfTaskForge_K2Node.h"
#include "EditorWidgetsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/LevelScriptBlueprint.h"

// --------------------------------------------------------------------------------------------------------------------

FTaskPaletteSummoner::FTaskPaletteSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor)
	: FWorkflowTabFactory("TaskPaletteID", BlueprintEditor)
	, WeakBlueprintEditor(BlueprintEditor)
{
	bIsSingleton = true;

	TabLabel = FText::FromString("Tasks Palette");
	TabIcon = FSlateIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Palette"));
}

TSharedRef<SWidget> FTaskPaletteSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SBtf_TaskPalette, WeakBlueprintEditor);
}

void SBtf_TaskPalette::Construct(const FArguments& InArgs, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	IsActiveTimerRegistered = false;
	BlueprintEditorPtr = InBlueprintEditor;

	// Create the asset discovery indicator
	FEditorWidgetsModule& EditorWidgetsModule = FModuleManager::LoadModuleChecked<FEditorWidgetsModule>("EditorWidgets");
	TSharedRef<SWidget> AssetDiscoveryIndicator = EditorWidgetsModule.CreateAssetDiscoveryIndicator(EAssetDiscoveryIndicatorScaleMode::Scale_Vertical);

	this->ChildSlot
		[
			SNew(SBorder)
			.Padding(2.0f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SAssignNew(GraphActionMenu, SGraphActionMenu)
						.OnActionDragged(this, &SBtf_TaskPalette::OnActionDragged)
						.OnCreateWidgetForAction(this, &SBtf_TaskPalette::OnCreateWidgetForAction)
						.OnCollectAllActions(this, &SBtf_TaskPalette::CollectAllActions)
						.AutoExpandActionMenu(true)
					]
					+SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Bottom)
					.Padding(FMargin(24, 0, 24, 0))
					[
						// Asset discovery indicator
						AssetDiscoveryIndicator
					]
					+SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Top)
					[
						SNew(SButton)
						.VAlign(VAlign_Center)
						.ContentPadding(FMargin(22,2))
						.Text(FText::FromString("Refresh"))
						.OnClicked_Lambda([this]() -> FReply
						{
							RefreshActionsList(true);
							return FReply::Handled();
						})
					]
				]
			]
		];

	// Register with the Asset Registry to be informed when it is done loading up files.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddSP(this, &SBtf_TaskPalette::RefreshActionsList, true);

	/**V: These delegates will be getting reworked in the future. Right now, calling CollectAllActions
	 * to ensure they always work is causing IMMENSE lag, because with each database action being updated,
	 * it refreshes ALL actions. Leading to the lag compounding (which was also multiplied by the amount
	 * of palette's open in other blueprints) and getting so severe that even blueprints were compiling
	 * slowly and the editor would take 3x the time to load.
	 * Optimally, whenever a specific action is updated, the specific action inside the palette is updated.
	 * But there doesn't seem to be any engine examples of how to do that. */

	// FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	// ActionDatabase.OnEntryRemoved().AddSP(this, &SBtf_TaskPalette::OnDatabaseActionsRemoved);
	// ActionDatabase.OnEntryUpdated().AddSP(this, &SBtf_TaskPalette::OnDatabaseActionsUpdated);
}

UBlueprint* SBtf_TaskPalette::GetBlueprint() const
{
	UBlueprint* BlueprintBeingEdited = NULL;
	if (BlueprintEditorPtr.IsValid())
	{
		BlueprintBeingEdited = BlueprintEditorPtr.Pin()->GetBlueprintObj();
	}
	return BlueprintBeingEdited;
}

void SBtf_TaskPalette::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	FBlueprintActionFilter::EFlags FilterFlags = FBlueprintActionFilter::BPFILTER_NoFlags;

	FBlueprintActionFilter MenuFilter(FilterFlags);
	MenuFilter.Context.Blueprints.Add(GetBlueprint());
	MenuFilter.Context.EditorPtr = BlueprintEditorPtr;

	auto RejectAnyIncompatibleReturnValues = [](const FBlueprintActionFilter& Filter, FBlueprintActionInfo& BlueprintAction)
	{
		if(BlueprintAction.NodeSpawner)
		{
			const UBtf_EditorSettings* DeveloperSettings = GetDefault<UBtf_EditorSettings>();

			//Add any functions that developers want added to the task palette.
			//This is most commonly done for functions that are related to tasks.
			if(DeveloperSettings->ExtraTaskPaletteFunctions.Contains(FName(BlueprintAction.NodeSpawner->DefaultMenuSignature.MenuName.ToString())))
			{
				return false;
			}

			if(BlueprintAction.NodeSpawner->NodeClass->IsChildOf(UBtf_TaskForge_K2Node::StaticClass())
				|| BlueprintAction.NodeSpawner->NodeClass.Get() == UBtf_TaskForge_K2Node::StaticClass())
			{
				/**This filter will remove the static functions (ExtendConstructObject, etc) from
				 * the UBtf_Utils_ExtendConstructObject library.
				 * It appears that if you add BlueprintInternalUseOnly, then the UFunction will
				 * become invalid. So we can filter it out based on if it's valid or not. */
				const UFunction* Function = BlueprintAction.GetAssociatedFunction();
				return !Function;
			}
		}

		return true;
	};

	MenuFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateLambda(RejectAnyIncompatibleReturnValues));

	FBlueprintActionMenuBuilder PaletteBuilder;
	/**The palette seems to break if we don't add at least one empty section*/
	PaletteBuilder.AddMenuSection(MenuFilter, FText::GetEmpty(), 0, FBlueprintActionMenuBuilder::ConsolidatePropertyActions);
	PaletteBuilder.RebuildActionList();
	OutAllActions.Append(PaletteBuilder);
}

void SBtf_TaskPalette::RefreshActionsList(bool bPreserveExpansion)
{
	SGraphPalette::RefreshActionsList(bPreserveExpansion);
}

TSharedRef<SWidget> SBtf_TaskPalette::OnCreateWidgetForAction(FCreateWidgetForActionData* const InCreateData)
{
	return SGraphPalette::OnCreateWidgetForAction(InCreateData);
}

FReply SBtf_TaskPalette::OnActionDragged(const TArray<TSharedPtr<FEdGraphSchemaAction>>& InActions,
	const FPointerEvent& MouseEvent)
{
	return SGraphPalette::OnActionDragged(InActions, MouseEvent);
}

TSharedRef<SVerticalBox> SBtf_TaskPalette::ConstructHeadingWidget(FSlateBrush const* const Icon, FText const& TitleText,
	FText const& ToolTipText)
{
	TSharedPtr<SToolTip> ToolTipWidget;
	SAssignNew(ToolTipWidget, SToolTip).Text(ToolTipText);

	static FTextBlockStyle TitleStyle = FTextBlockStyle()
		.SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		.SetColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f));

	return SNew(SVerticalBox)
		.ToolTip(ToolTipWidget)
		.Visibility(EVisibility::Visible)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 2.f)
			[
				SNew(SImage).Image(Icon)
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 2.f)
			[
				SNew(STextBlock)
				.Text(TitleText)
				.TextStyle(&TitleStyle)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 2.f, 0.f, 5.f)
		[
			SNew(SBorder)
			.Padding(1.f)
			.BorderImage(FAppStyle::GetBrush(TEXT("Menu.Separator")))
		];
}

void SBtf_TaskPalette::OnDatabaseActionsUpdated(UObject* ActionsKey)
{
	if(ActionsKey->IsA(UBtf_TaskForge::StaticClass()))
	{
		if(GraphActionMenu.IsValid())
		{
			GraphActionMenu->RefreshAllActions(true);
		}
	}
}

void SBtf_TaskPalette::OnDatabaseActionsRemoved(UObject* ActionsKey)
{
	if(ActionsKey->IsA(UBtf_TaskForge::StaticClass()))
	{
		ULevelScriptBlueprint* RemovedLevelScript = Cast<ULevelScriptBlueprint>(ActionsKey);
		bool const bAssumeDestroyingWorld = (RemovedLevelScript != nullptr);

		// if (bAssumeDestroyingWorld)
		// {
		// 	// have to update the action list immediatly (cannot wait until Tick(),
		// 	// because we have to handle level switching, which expects all references
		// 	// to be cleared immediately)
		// 	if(GraphActionMenu.IsValid())
		// 	{
		// 		GraphActionMenu->RefreshAllActions(true);
		// 	}
		// }
		// else
		// {
		// 	if(GraphActionMenu.IsValid())
		// 	{
		// 		GraphActionMenu->RefreshAllActions(true);
		// 	}
		// }
	}
}

// --------------------------------------------------------------------------------------------------------------------
