//	Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_ExtendConstructObject.h"
#include "K2Node_ExtendSpawnActor.generated.h"

/**
 * 
 */
UCLASS()
class BLUEPRINTNODETEMPLATEEDITOR_API UK2Node_ExtendSpawnActor : public UK2Node_ExtendConstructObject
{
	GENERATED_BODY()
public:
	UK2Node_ExtendSpawnActor(const FObjectInitializer& ObjectInitializer);

	virtual void AllocateDefaultPins() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetCornerIcon() const override
	{
		if (ProxyClass)
		{
			if (const auto Btt = ProxyClass->GetDefaultObject())
			{
				if (OutDelegate.Num() == 0 /*&& InExecPinsMap.Num() == 0*/)
				{
					return FName();
				}
			}
		}
		return TEXT("Graph.Latent.LatentIcon");
	}

protected:
	UEdGraphPin* GetSpawnTransformPin() const;
	UEdGraphPin* GetCollisionHandlingOverridePin() const;
};
