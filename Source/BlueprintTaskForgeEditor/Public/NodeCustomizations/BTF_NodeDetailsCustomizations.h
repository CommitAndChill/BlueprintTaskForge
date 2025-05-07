#pragma once
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"

class FBNTNodeDetailsCustomizations : public IDetailCustomization
{
public:

	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	
};
