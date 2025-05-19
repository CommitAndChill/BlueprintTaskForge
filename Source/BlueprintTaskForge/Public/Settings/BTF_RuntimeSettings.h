#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Btf_RuntimeSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, DisplayName = "Blueprint Task Forge Runtime Settings")
class BLUEPRINTTASKFORGE_API UBtf_RuntimeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	/**Inside a tasks class defaults, should "Deactivate" always be
	 * enforced in the "Exec Function" array? */
	UPROPERTY(Category = "Node Settings", EditAnywhere, Config)
	bool EnforceDeactivateExecFunction = true;

	virtual FName GetSectionName() const override
	{
		return "Blueprint Task Forge Runtime Settings";
	}

	virtual FName GetCategoryName() const override
	{
		return "Plugins";
	}
};
