#pragma once
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"

class FBtf_NodeDetailsCustomizations : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
