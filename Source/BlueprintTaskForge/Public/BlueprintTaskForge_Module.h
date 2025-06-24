#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Misc/Guid.h"
#include "Serialization/CustomVersion.h"

// --------------------------------------------------------------------------------------------------------------------

class FBlueprintTaskForgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

// --------------------------------------------------------------------------------------------------------------------

struct FBlueprintTaskForgeCustomVersion
{
    enum Type
    {
        BeforeCustomVersionWasAdded = 0,
        ExposeOnSpawnInClass = 1,
        Btf_UE5_1,
        VersionPlusOne,
        LatestVersion = VersionPlusOne - 1
    };

    BLUEPRINTTASKFORGE_API const static FGuid GUID;
};

// --------------------------------------------------------------------------------------------------------------------