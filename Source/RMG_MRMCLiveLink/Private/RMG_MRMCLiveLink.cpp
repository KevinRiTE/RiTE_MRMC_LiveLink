// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "RMG_MRMCLiveLink.h"

#define LOCTEXT_NAMESPACE "FRMG_MRMCLiveLinkModule"

void FRMG_MRMCLiveLinkModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRMG_MRMCLiveLinkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRMG_MRMCLiveLinkModule, RMG_MRMCLiveLink)