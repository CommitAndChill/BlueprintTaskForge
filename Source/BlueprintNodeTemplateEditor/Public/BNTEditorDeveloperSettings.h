#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BNTEditorDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Blueprint Tasks Editor Settings"))
class BLUEPRINTNODETEMPLATEEDITOR_API UBNTEditorDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/**The task palette will automatically retrieve all task nodes,
	 * but your project might contain functions that are related
	 * to tasks that you want to add to the task palette.
	 * This array allows you to add whatever function you
	 * want to that palette. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly)
	TArray<FName> ExtraTaskPaletteFunctions;

	virtual FName GetSectionName() const override
	{
		return "Blueprint Tasks Editor Settings";
	}

	virtual FName GetCategoryName() const override
	{
		return "Plugins";
	}
};
