// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SRMG_MRMCLiveLinkSourceFactory.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RMG_MRMCLiveLinkSourceEditor"

void SRMG_MRMCLiveLinkSourceFactory::Construct(const FArguments& Args)
{
	OkClicked = Args._OnOkClicked;

	FIPv4Endpoint Endpoint;
//	Endpoint.Address = FIPv4Address::Any;
	FIPv4Address::Parse("0.0.0.0", Endpoint.Address);
	Endpoint.Port = 55535;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(250)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[

			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(0.40)
			[
				SNew(STextBlock).Text(LOCTEXT("Udp Connects", "Udp Connect"))
			]

		   + SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			[

				SNew(SCheckBox)
				.IsChecked(this, &SRMG_MRMCLiveLinkSourceFactory::GetConnectedBody)
			    .OnCheckStateChanged(this, &SRMG_MRMCLiveLinkSourceFactory::ConnectedBodyChanged, &_checkValUdp)

			]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.FillWidth(0.5f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("JSONPortNumber", "Port Number"))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(0.5f)
				[
					SAssignNew(EditabledText, SEditableTextBox)
					.Text(FText::FromString(Endpoint.ToString()))
					.OnTextCommitted(this, &SRMG_MRMCLiveLinkSourceFactory::OnEndpointChanged)
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew(SButton)
				.OnClicked(this, &SRMG_MRMCLiveLinkSourceFactory::OnOkClicked)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Ok", "Ok"))
				]
			]
		]
	];
}

void SRMG_MRMCLiveLinkSourceFactory::OnEndpointChanged(const FText& NewValue, ETextCommit::Type)
{
	TSharedPtr<SEditableTextBox> EditabledTextPin = EditabledText.Pin();
	if (EditabledTextPin.IsValid())
	{
		FIPv4Endpoint Endpoint;
		if (!FIPv4Endpoint::Parse(NewValue.ToString(), Endpoint))
		{
			//Endpoint.Address = FIPv4Address::Any;
			FIPv4Address::Parse("239.255.1.2", Endpoint.Address);
			//Endpoint.Port = 54321;
			Endpoint.Port = 24680;
			EditabledTextPin->SetText(FText::FromString(Endpoint.ToString()));
		}
	}
}

FReply SRMG_MRMCLiveLinkSourceFactory::OnOkClicked()
{
	TSharedPtr<SEditableTextBox> EditabledTextPin = EditabledText.Pin();
	if (EditabledTextPin.IsValid())
	{
		FIPv4Endpoint Endpoint;
		if (FIPv4Endpoint::Parse(EditabledTextPin->GetText().ToString(), Endpoint))
		{
			OkClicked.ExecuteIfBound(Endpoint);
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE