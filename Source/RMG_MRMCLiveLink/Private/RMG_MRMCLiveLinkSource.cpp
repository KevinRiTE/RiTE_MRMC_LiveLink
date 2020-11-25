// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "RMG_MRMCLiveLinkSource.h"
#include <cmath>

#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Roles/LiveLinkAnimationTypes.h"

#include "Async/Async.h"
#include "Common/UdpSocketBuilder.h"
#include "HAL/RunnableThread.h"
#include "Json.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include <chrono>
#include "RenderCore.h"

#define LOCTEXT_NAMESPACE "RMG_MRMCLiveLinkSource"

#define RECV_BUFFER_SIZE 1024 * 1024

const FString version = "Version 0.1.12";

using namespace std::chrono;


FRMG_MRMCLiveLinkSource::FRMG_MRMCLiveLinkSource(FIPv4Endpoint InEndpoint)
: Socket(nullptr)
, Stopping(false)
, Thread(nullptr)
, isRunning(false)
, WaitTime(FTimespan::FromMilliseconds(100))
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), *version);
	// defaults
	DeviceEndpoint = InEndpoint;
    FIPv4Address::Parse("0.0.0.0", DeviceEndpoint.Address);
    DeviceEndpoint.Port = 55535;

	SourceStatus = LOCTEXT("SourceStatus_DeviceNotFound", "Device Not Found");
	SourceType = LOCTEXT("RMG_MRMCLiveLinkSourceType", "RMG MRMC LiveLink");
	SourceMachineName = LOCTEXT("RMG_MRMCLiveLinkSourceMachineName", "localhost");

	//setup socket
	if (DeviceEndpoint.Address.IsMulticastAddress())
	{
		Socket = FUdpSocketBuilder(TEXT("JSONSOCKET"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToPort(DeviceEndpoint.Port)
			.WithReceiveBufferSize(RECV_BUFFER_SIZE)

			.BoundToAddress(FIPv4Address::Any)
			.JoinedToGroup(DeviceEndpoint.Address)
			.WithMulticastLoopback()
			.WithMulticastTtl(2);
					
	}
	else
	{
		Socket = FUdpSocketBuilder(TEXT("JSONSOCKET"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToAddress(DeviceEndpoint.Address)
			.BoundToPort(DeviceEndpoint.Port)
			.WithReceiveBufferSize(RECV_BUFFER_SIZE);
	}

	RecvBuffer.SetNumUninitialized(RECV_BUFFER_SIZE);

	if ((Socket != nullptr) && (Socket->GetSocketType() == SOCKTYPE_Datagram))
	{
		SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

		Start();

		SourceStatus = LOCTEXT("SourceStatus_Receiving", "Receiving");
        isRunning = true;
	}
}

FRMG_MRMCLiveLinkSource::~FRMG_MRMCLiveLinkSource()
{
	Stop();
	if (Thread != nullptr)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
	if (Socket != nullptr)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        isRunning = false;
	}
}

void FRMG_MRMCLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	Client = InClient;
	SourceGuid = InSourceGuid;
}


bool FRMG_MRMCLiveLinkSource::IsSourceStillValid() const
{
	// Source is valid if we have a valid thread and socket
	bool bIsSourceValid = !Stopping && Thread != nullptr && Socket != nullptr;
	return bIsSourceValid;
}


bool FRMG_MRMCLiveLinkSource::RequestSourceShutdown()
{
	Stop();

	return true;
}
// FRunnable interface

void FRMG_MRMCLiveLinkSource::Start()
{
	ThreadName = "RMG_MRMC UDP Receiver ";
	ThreadName.AppendInt(FAsyncThreadIndex::GetNext());
	
	Thread = FRunnableThread::Create(this, *ThreadName, 128 * 1024, TPri_AboveNormal, FPlatformAffinity::GetPoolThreadMask());
}

void FRMG_MRMCLiveLinkSource::Stop()
{
	Stopping = true;
}

uint32 FRMG_MRMCLiveLinkSource::Run()
{
	TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();
	
	while (!Stopping)
	{
		if (Socket->Wait(ESocketWaitConditions::WaitForRead, WaitTime))
		{
			uint32 Size;

			while (Socket->HasPendingData(Size))
			{
				int32 Read = 0;

				if (Socket->RecvFrom(RecvBuffer.GetData(), RecvBuffer.Num(), Read, *Sender))
				{
					if (Read > 0)
					{
						TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData = MakeShareable(new TArray<uint8>());
						ReceivedData->SetNumUninitialized(Read);
						memcpy(ReceivedData->GetData(), RecvBuffer.GetData(), Read);
						AsyncTask(ENamedThreads::GameThread, [this, ReceivedData]() { HandleReceivedData(ReceivedData); });
					}
				}
			}
		}
	}
	return 0;
}
void  FRMG_MRMCLiveLinkSource::SetupSubjects(const FString JsonString, TArray<Subject> &SubjectList)
{

    FString InputJson =
R"({ "sources": [{ 
         "subject": "robot_camera", 
             "properties": ["Roll", "Focus", "Zoom"],
             "propertyIndex": [6, 7, 8],
             "bones" : [{ 
                 "name": "top", 
                 "parent" : ""  ,
                 "index": [-1, -1, -1, -1, -1, -1]
              }, 
              { 
                 "name": "CameraPose", 
                 "parent" : "top",
                 "index": [0, 1, 2, 3, 4, 5]
              }] 
         },
         { 
         "subject": "camera_target",
            "properties": ["CameraTarget_xt", "CameraTarget_yt", "CameraTarget_zt"], 
             "propertyIndex": [9, 10, 11],
            "bones" : [{ 
                 "name": "top", 
                 "parent" : "" ,
                 "index": [-1, -1, -1, -1, -1, -1]
            }, 
            { 
                 "name": "CameraTarget", 
                 "parent" : "top" ,
                 "index": [ 9, 10, 11, -1, -1, -1]
            }]
         }] 
})";



   // UE_LOG(LogTemp, Warning, TEXT("%s"), *InputJson);
    UE_LOG(LogTemp, Warning, TEXT("%s"), *InputJson);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InputJson);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        auto SubjectArray = JsonObject->GetArrayField(TEXT("sources"));
        for (auto Subj : SubjectArray) {
            Subject NewSubject;
            auto SubjectObject = Subj->AsObject();
            FName SubjectName = *SubjectObject->GetStringField("subject");
            NewSubject.SubjectName = SubjectName;

            FLiveLinkStaticDataStruct StaticDataStruct = FLiveLinkStaticDataStruct(FLiveLinkSkeletonStaticData::StaticStruct());
            FLiveLinkSkeletonStaticData& StaticData = *StaticDataStruct.Cast<FLiveLinkSkeletonStaticData>();
            Client->RemoveSubject_AnyThread({ SourceGuid, SubjectName });

            auto BoneArray = SubjectObject->GetArrayField(TEXT("bones"));

            StaticData.BoneNames.Reset(BoneArray.Num());
            StaticData.BoneParents.Reset(BoneArray.Num());

            FString sname;
            int BoneIdx = 0;
            for (auto BoneItem : BoneArray) {
                Bone NewBone;
                auto BoneObject = BoneItem->AsObject();

                FString BoneName;
                if (BoneObject->TryGetStringField(TEXT("name"), BoneName))
                {
                    NewBone.Name = BoneName;
                    NewBone.Id = BoneIdx;
                }

                FString BoneParent;
                if (BoneObject->TryGetStringField("parent", BoneParent))
                {
                    NewBone.ParentName = BoneParent;
                    if (!BoneParent.IsEmpty()) {
                        NewBone.IsRoot = false;
                    }
                }
                auto IndexArray = BoneObject->GetArrayField(TEXT("index"));
                for (int i = 0; i < 6; i++) {
                    if (i < IndexArray.Num()) {
                        NewBone.Index[i] = IndexArray[i]->AsNumber();
                        //UE_LOG(LogTemp, Warning, TEXT("Index:%d"), NewBone.Index[i]);
                    }
                }
                BoneIdx++;
                NewSubject.Bones.Add(NewBone);

                StaticData.BoneNames.Add(*NewBone.Name);
                auto parent = NewBone.IsRoot ? INDEX_NONE : 0;
                StaticData.BoneParents.Add(parent);
            }
            auto PropertyArray = SubjectObject->GetArrayField(TEXT("properties"));
            auto PropertyIndexArray = SubjectObject->GetArrayField(TEXT("propertyIndex"));
            int pcnt = 0;
            for (auto Prop : PropertyArray) {
                BoneProperty NewBoneProperty;
                NewBoneProperty.Name = Prop->AsString();
                UE_LOG(LogTemp, Warning, TEXT("Property:%s"), *NewBoneProperty.Name);
                if (pcnt < PropertyIndexArray.Num()) {
                    NewBoneProperty.Index = PropertyIndexArray[pcnt++]->AsNumber();
                    //UE_LOG(LogTemp, Warning, TEXT("Index:%d"), NewBoneProperty.Index);
                }
                NewSubject.Properties.Add(NewBoneProperty);
                StaticData.PropertyNames.Add(*NewBoneProperty.Name);
            }
            SubjectList.Add(NewSubject);

            Client->PushSubjectStaticData_AnyThread({ SourceGuid, SubjectName },
                ULiveLinkAnimationRole::StaticClass(),
                MoveTemp(StaticDataStruct));

        }
    }
}

static bool SkipFrame(FQualifiedFrameTime &SceneTime)
{
    double CurrentSeconds = FPlatformTime::Seconds();
    FFrameRate FrameRate = FApp::GetTimecodeFrameRate();
    FTimecode TimeCode = FTimecode(CurrentSeconds, FrameRate, true);
    SceneTime = FQualifiedFrameTime(TimeCode,FrameRate);

    static double LastSeconds = 0.0;
    //static high_resolution_clock::time_point Time1 = high_resolution_clock::now();

    //UE_LOG(LogTemp, Warning, TEXT("source time: %f"), CurrentSeconds);
    //UE_LOG(LogTemp, Warning, TEXT("Timecode: %s"), *TimeCode.ToString());

    double SecondRate = 1.0 / static_cast<double>(FrameRate.Numerator);

    high_resolution_clock::time_point Time2 = high_resolution_clock::now();

    //duration<double> time_span = duration_cast<duration<double>>(Time2 - Time1);
    //UE_LOG(LogTemp, Warning, TEXT("delta: %f, time span:%f"), CurrentSeconds - LastSeconds,time_span.count());
    if (CurrentSeconds < LastSeconds + SecondRate)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Skipping             %f"), CurrentSeconds);
        return true;
    } else 
    {
    //UE_LOG(LogTemp, Warning, TEXT("Frame rate 1.0/%d"), FrameRate.Numerator);
        LastSeconds = CurrentSeconds;
        //Time1 = Time2;
    }
    return false;
}

void FRMG_MRMCLiveLinkSource::SendFrameToLiveLink(const TArray<Subject> SubjectList, const TArray<float> FrameValueList)
{

    FQualifiedFrameTime SceneTime;
    if (SkipFrame(SceneTime))
    {
        return;
    }
    for (auto Subj : SubjectList) {
        FName SubjectName = Subj.SubjectName;
        //UE_LOG(LogTemp, Warning, TEXT("SubjectList: %s"), *SubjectName.ToString());
        FLiveLinkFrameDataStruct FrameDataStruct = FLiveLinkFrameDataStruct(FLiveLinkAnimationFrameData::StaticStruct());
        FLiveLinkAnimationFrameData& FrameData = *FrameDataStruct.Cast<FLiveLinkAnimationFrameData>();

        FrameData.Transforms.Reserve(Subj.Bones.Num());  
        FrameData.WorldTime = FPlatformTime::Seconds();
        FrameData.MetaData.SceneTime = SceneTime;

        for (auto Bone : Subj.Bones) {
            //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
            //UE_LOG(LogTemp, Warning, TEXT("Parent:%s id:%d"), *Bone.ParentName,Bone.Id);
            FTransform Trans;
            FVector Location(0.0f, 0.0f, 0.0f);
            FVector Rotation(0.0f, 0.0f, 0.0f);
            int ValueSize = FrameValueList.Num();
            int Xdx = Bone.Index[0];
            int Ydx = Bone.Index[1];
            int Zdx = Bone.Index[2];
            if (Xdx > -1 && Ydx > -1 && Zdx > -1 &&
                Xdx < ValueSize && Ydx < ValueSize && Zdx < ValueSize) {
                Location.Set(FrameValueList[Xdx], FrameValueList[Ydx], FrameValueList[Zdx]);
                //UE_LOG(LogTemp, Warning, TEXT("Loc:%f %f %f"), Location.X,Location.Y,Location.Z);
                //UE_LOG(LogTemp, Warning, TEXT("Rot:%f %f %f"), Rotation.X,Rotation.Y,Rotation.Z);
                if (Bone.Name == "CameraPose") {
                    //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
                    double CurrentSeconds = FPlatformTime::Seconds();
                    static double LastCurrentSeconds = 0.0;
                    //UE_LOG(LogTemp, Warning, TEXT("%0.4f,%0.2f,%0.2f,%0.2f"), CurrentSeconds - LastCurrentSeconds, Location.X, Location.Y, Location.Z);
                    LastCurrentSeconds = CurrentSeconds;
                }
            }
            int rXdx = Bone.Index[3];
            int rYdx = Bone.Index[4];
            int rZdx = Bone.Index[5];
            if (rXdx > -1 && rYdx > -1 && rZdx > -1 &&
                rXdx < ValueSize && rYdx < ValueSize && rZdx < ValueSize) {
                Rotation.Set(FrameValueList[rXdx], FrameValueList[rYdx], FrameValueList[rZdx]);
            }
            Trans.SetLocation(Location);
            if (Rotation.Size() > 0) {
                Trans.SetRotation(FQuat::MakeFromEuler(Rotation));
                //UE_LOG(LogTemp, Warning, TEXT("Name:%s"), *Bone.Name);
                //UE_LOG(LogTemp, Warning, TEXT("Rot:%f %f %f"), Rotation.X,Rotation.Y,Rotation.Z);
            }
            FrameData.Transforms.Add(Trans);
        }
        for (auto Prop : Subj.Properties) {
            if (Prop.Index > -1 && Prop.Index < FrameValueList.Num()) {
                float Value = FrameValueList[Prop.Index];
                //UE_LOG(LogTemp, Warning, TEXT("Property:%s index:%d"), *Prop.Name, Prop.Index);
                //UE_LOG(LogTemp, Warning, TEXT("Value %f"), value);
                FrameData.PropertyValues.Add(Value);
            }
        }
        Client->PushSubjectFrameData_AnyThread({ SourceGuid, SubjectName }, MoveTemp(FrameDataStruct));
    }
}

void FRMG_MRMCLiveLinkSource::HandleReceivedData(TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> ReceivedData)
{
    if (Stopping) {
        return; // thread is shutting down
    }
	FString JsonString;
	RobotData Robot_Data;
	JsonString.Empty(ReceivedData->Num());
	for (uint8& Byte : *ReceivedData.Get())
	{
		JsonString += TCHAR(Byte);
	}

	if (ReceivedData->Num() > 35) {
		memcpy(&Robot_Data, ReceivedData->GetData(), 36);
	}

    if (NeedSubjectSeup) {
        SetupSubjects(FString(""), Subjects);
        NeedSubjectSeup = false;
    }
    FrameValues.Empty(); // init the FrameValues
    // first lets manually setup Subj Values
	FVector CameraPose;
	FVector CameraTarget;
	const float METER2CENT = 100.0f;

    // adjust Robot_data for different coord system
    Robot_Data.yv = -Robot_Data.yv; // flip Y axis
    Robot_Data.yt = -Robot_Data.yt; // flip Y axis


    CameraPose = FVector(Robot_Data.xv * METER2CENT,
                         Robot_Data.yv * METER2CENT,
                         Robot_Data.zv * METER2CENT); // CameraPose
    CameraTarget = FVector(Robot_Data.xt * METER2CENT,
                           Robot_Data.yt * METER2CENT,
                           Robot_Data.zt * METER2CENT);
    FVector LookAt = CameraTarget - CameraPose;
    float PanX = LookAt.X;
    float PanY = LookAt.Y;
    float Pan = FMath::RadiansToDegrees(atan2(PanY, PanX)); // atan2 returns radians // flip z rotation for pan
    float TiltX = FVector(LookAt.X, LookAt.Y, 0.0).Size();
    float TiltY = LookAt.Z;
    float Tilt = FMath::RadiansToDegrees(atan2(TiltY, TiltX)); // atan2 returns radians
    float Roll = -Robot_Data.roll;  //reverse roll for UE after test

    FrameValues.Add(CameraPose.X);     // 0
    FrameValues.Add(CameraPose.Y);     // 1
    FrameValues.Add(CameraPose.Z);     // 2
    // roll is xrot, tilt is yrot, pan is zrot
    FrameValues.Add(FMath::RadiansToDegrees(Roll));             // 3
    FrameValues.Add(Tilt);             // 4
    FrameValues.Add(Pan);              // 5
    FrameValues.Add(Robot_Data.roll);  // 6
    //FrameValues.Add(Robot_Data.focus * METER2CENT); // 7
    FrameValues.Add(LookAt.Size()); // 7  // use LookAt because Robot focus distance is wrong
    FrameValues.Add(Robot_Data.zoom);  // 8
    FrameValues.Add(CameraTarget.X);  // 9
    FrameValues.Add(CameraTarget.Y);  // 10 
    FrameValues.Add(CameraTarget.Z);  // 11 

    SendFrameToLiveLink(Subjects, FrameValues);
}
#undef LOCTEXT_NAMESPACE
