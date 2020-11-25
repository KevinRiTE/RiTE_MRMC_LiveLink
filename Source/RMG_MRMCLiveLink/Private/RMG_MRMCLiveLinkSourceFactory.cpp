// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#include "RMG_MRMCLiveLinkSourceFactory.h"
#include "RMG_MRMCLiveLinkSource.h"
#include "SRMG_MRMCLiveLinkSourceFactory.h"

#define LOCTEXT_NAMESPACE "RMG_MRMCLiveLinkSourceFactory"

FText URMG_MRMCLiveLinkSourceFactory::GetSourceDisplayName() const
{
	return LOCTEXT("SourceDisplayName", "RMG_MRMC LiveLink");
}

FText URMG_MRMCLiveLinkSourceFactory::GetSourceTooltip() const
{
	return LOCTEXT("SourceTooltip", "Creates a connection to a RMG_MRMC UDP Stream");
}

TSharedPtr<SWidget> URMG_MRMCLiveLinkSourceFactory::BuildCreationPanel(FOnLiveLinkSourceCreated InOnLiveLinkSourceCreated) const
{
	return SNew(SRMG_MRMCLiveLinkSourceFactory)
		.OnOkClicked(SRMG_MRMCLiveLinkSourceFactory::FOnOkClicked::CreateUObject(this, &URMG_MRMCLiveLinkSourceFactory::OnOkClicked, InOnLiveLinkSourceCreated));
}

TSharedPtr<ILiveLinkSource> URMG_MRMCLiveLinkSourceFactory::CreateSource(const FString& InConnectionString) const
{
	FIPv4Endpoint DeviceEndPoint;
	if (!FIPv4Endpoint::Parse(InConnectionString, DeviceEndPoint))
	{
		return TSharedPtr<ILiveLinkSource>();
	}

	return MakeShared<FRMG_MRMCLiveLinkSource>(DeviceEndPoint);
}

void URMG_MRMCLiveLinkSourceFactory::OnOkClicked(FIPv4Endpoint InEndpoint, FOnLiveLinkSourceCreated InOnLiveLinkSourceCreated) const
{
	InOnLiveLinkSourceCreated.ExecuteIfBound(MakeShared<FRMG_MRMCLiveLinkSource>(InEndpoint), InEndpoint.ToString());
}

#undef LOCTEXT_NAMESPACE