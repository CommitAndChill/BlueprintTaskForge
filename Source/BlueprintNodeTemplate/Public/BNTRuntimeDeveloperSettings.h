#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BNTRuntimeDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Blueprint Tasks Runtime Settings")
class BLUEPRINTNODETEMPLATE_API UBNTRuntimeDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/**Inside a tasks class defaults, should "Deactivate" always be
	 * enforced in the "Exec Function" array? */
	UPROPERTY(Category = "Node Settings", EditAnywhere, Config)
	bool EnforceDeactivateExecFunction = true;

	virtual FName GetSectionName() const override
	{
		return "Blueprint Tasks Runtime Settings";
	}

	virtual FName GetCategoryName() const override
	{
		return "Plugins";
	}
};
