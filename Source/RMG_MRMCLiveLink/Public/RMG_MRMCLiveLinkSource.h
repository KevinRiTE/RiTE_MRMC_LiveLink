// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ILiveLinkSource.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "IMessageContext.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

class FRunnableThread;
class FSocket;
class ILiveLinkClient;
class ISocketSubsystem;


struct Bone {
    FString Name;
    FString ParentName;
    FTransform BoneTransform;
    int Index[6] = { -1,-1,-1,-1,-1,-1 };
    bool IsRoot = true;
    int Id=0;
};

struct BoneProperty {
    FString Name;
    int Index = -1;
};

struct Subject {
    FName SubjectName;
    TArray<Bone> Bones;
    TArray<BoneProperty> Properties;
    TArray<float> Values;
};
struct RobotData {
	float xv = 0.0f;
	float yv = 0.0f;
	float zv = 0.0f;
	float xt = 0.0f;
	float yt = 0.0f;
	float zt = 0.0f;
	float roll = 0.0f;
	float focus = 0.0f;
	float zoom = 0.0f;
};

class RMG_MRMCLIVELINK_API FRMG_MRMCLiveLinkSource : public ILiveLinkSource, public FRunnable
{
public:

	FRMG_MRMCLiveLinkSource(FIPv4Endpoint Endpoint);

	virtual ~FRMG_MRMCLiveLinkSource();

	// Begin ILiveLinkSource Interface
	
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override;

	virtual bool IsSourceStillValid() const override;

	virtual bool RequestSourceShutdown() override;

	virtual FText GetSourceType() const override { return SourceType; };
	virtual FText GetSourceMachineName() const override { return SourceMachineName; }
	virtual FText GetSourceStatus() const override { return SourceStatus; }

	// End ILiveLinkSource Interface

	// Begin FRunnable Interface

	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	void Start();
	virtual void Stop() override;
	virtual void Exit() override { }

	// End FRunnable Interface

	void HandleReceivedData(TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData);
    void SetupSubjects(const FString JsonString, TArray<Subject> &Subjects);
    void SendFrameToLiveLink(const TArray<Subject> Subjects, const TArray<float> FrameValues);

private:

	ILiveLinkClient* Client;

	// Our identifier in LiveLink
	FGuid SourceGuid;

	FMessageAddress ConnectionAddress;

	FText SourceType;
	FText SourceMachineName;
	FText SourceStatus;

	FIPv4Endpoint DeviceEndpoint;

	// Socket to receive data on
	FSocket* Socket;

	// Subsystem associated to Socket
	ISocketSubsystem* SocketSubsystem;

	// Threadsafe Bool for terminating the main thread loop
	FThreadSafeBool Stopping;

	// Thread to run socket operations on
	FRunnableThread* Thread;

	// Name of the sockets thread
	FString ThreadName;

	// Time to wait between attempted receives
	FTimespan WaitTime;

	// List of subjects we've already encountered
	TSet<FName> EncounteredSubjects;

	// Buffer to receive socket data into
	TArray<uint8> RecvBuffer;

    bool NeedSubjectSeup = true;
    bool isRunning = false;
    TArray<Subject> Subjects;
    TArray<float> FrameValues;
};
