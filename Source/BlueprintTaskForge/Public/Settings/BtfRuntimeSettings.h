#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BftMacros.h"
#include "BtfRuntimeSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, DisplayName = "Blueprint Task Forge Runtime Settings")
class BLUEPRINTTASKFORGE_API UBtf_RuntimeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetSectionName() const override;

    virtual FName GetCategoryName() const override;

private:
    /**Inside a tasks class defaults, should "Deactivate" always be
     * enforced in the "Exec Function" array? */
    UPROPERTY(Category = "Node Settings", EditAnywhere, Config, meta = (AllowPrivateAccess = "true"))
    bool EnforceDeactivateExecFunction = true;

public:
    BFT_PROPERTY_GET(EnforceDeactivateExecFunction)
};

// --------------------------------------------------------------------------------------------------------------------
