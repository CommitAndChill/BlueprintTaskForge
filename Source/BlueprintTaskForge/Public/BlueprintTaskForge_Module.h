#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"
#include "Misc/Guid.h"
#include "Serialization/CustomVersion.h"

class FBlueprintTaskForgeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

struct FBlueprintTaskForgeCustomVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		ExposeOnSpawnInClass = 1,
		// -----<new versions can be added above this line>-------------------------------------------------
		BTF_UE5_1,
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
	// The GUID for this custom version number
	BLUEPRINTTASKFORGE_API const static FGuid GUID;
};