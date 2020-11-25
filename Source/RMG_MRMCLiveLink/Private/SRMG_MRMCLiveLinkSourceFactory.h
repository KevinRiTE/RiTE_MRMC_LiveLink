// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Types/SlateEnums.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SEditableTextBox;

class SRMG_MRMCLiveLinkSourceFactory : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnOkClicked, FIPv4Endpoint);

	SLATE_BEGIN_ARGS(SRMG_MRMCLiveLinkSourceFactory){}
		SLATE_EVENT(FOnOkClicked, OnOkClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:

	void OnEndpointChanged(const FText& NewValue, ETextCommit::Type);

	FReply OnOkClicked();

	bool _checkValUdp = true;
	ECheckBoxState GetConnectedBody()const { return (_checkValUdp==true) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };
	void ConnectedBodyChanged(ECheckBoxState state, bool* value) { _checkValUdp = (state == ECheckBoxState::Checked); }

	TWeakPtr<SEditableTextBox> EditabledText;
	FOnOkClicked OkClicked;
};