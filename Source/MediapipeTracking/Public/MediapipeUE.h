#pragma once
//#include "Engine.h"
#include "Modules/ModuleManager.h"

class FMEDIAPIPETRACKINGModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};