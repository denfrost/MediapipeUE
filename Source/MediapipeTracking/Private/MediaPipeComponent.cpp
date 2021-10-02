// Fill out your copyright notice in the Description page of Project Settings.


#include "MediaPipeComponent.h"
#include "Math/Quat.h"

// Sets default values for this component's properties
UMediaPipeComponent::UMediaPipeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	Native = MakeShareable(new FMediapipeMessenger);
	LinkupCallbacks();
}


// Called when the game starts
void UMediaPipeComponent::BeginPlay()
{
	Super::BeginPlay();

	Native->Settings = Settings;

	if (Settings.bShouldAutoOpenSend)
	{
		OpenSendSocket(Settings.SendIP, Settings.SendPort);
	}

	if (Settings.bShouldAutoOpenReceive)
	{
		OpenReceiveSocket(Settings.ReceiveIP, Settings.ReceivePort);
	}
	
}


// Called every frame
void UMediaPipeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}
void UMediaPipeComponent::LinkupCallbacks()
{
	Native->OnSendOpened = [this](int32 SpecifiedPort, int32 BoundPort)
	{
		Settings.bIsSendOpen = true;
		Settings.SendBoundPort = BoundPort;	//ensure sync on opened bound port

		Settings.SendIP = Native->Settings.SendIP;
		Settings.SendPort = Native->Settings.SendPort;

		OnSendSocketOpened.Broadcast(Settings.SendPort, Settings.SendBoundPort);
	};
	Native->OnSendClosed = [this](int32 Port)
	{
		Settings.bIsSendOpen = false;
		OnSendSocketClosed.Broadcast(Port);
	};
	Native->OnReceiveOpened = [this](int32 Port)
	{
		Settings.ReceiveIP = Native->Settings.ReceiveIP;
		Settings.ReceivePort = Native->Settings.ReceivePort;

		Settings.bIsReceiveOpen = true;
		OnReceiveSocketOpened.Broadcast(Port);
	};
	Native->OnReceiveClosed = [this](int32 Port)
	{
		Settings.bIsReceiveOpen = false;
		OnReceiveSocketClosed.Broadcast(Port);
	};
	Native->OnReceivedBytes = [this](const FMediapipeTracking& Data)
	{
		OnReceivedData.Broadcast(Data);
	};
}

bool UMediaPipeComponent::CloseReceiveSocket()
{
	return Native->CloseReceiveSocket();
}

int32 UMediaPipeComponent::OpenSendSocket(const FString& InIP, const int32 InPort)
{
	//Sync side effect sampled settings
	Native->Settings.SendSocketName = Settings.SendSocketName;
	Native->Settings.BufferSize = Settings.BufferSize;

	return Native->OpenSendSocket(InIP, InPort);
}

bool UMediaPipeComponent::CloseSendSocket()
{
	Settings.SendBoundPort = 0;
	return Native->CloseSendSocket();
}

bool UMediaPipeComponent::OpenReceiveSocket(const FString& InListenIp /*= TEXT("0.0.0.0")*/, const int32 InListenPort /*= 3002*/)
{
	//Sync side effect sampled settings
	Native->Settings.bShouldAutoOpenReceive = Settings.bShouldAutoOpenReceive;
	Native->Settings.ReceiveSocketName = Settings.ReceiveSocketName;
	Native->Settings.BufferSize = Settings.BufferSize;

	return Native->OpenReceiveSocket(InListenIp, InListenPort);
}

bool UMediaPipeComponent::EmitBytes(const TArray<uint8>& Bytes)
{
	return Native->EmitBytes(Bytes);
}

void UMediaPipeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseSendSocket();
	CloseReceiveSocket();

	Native->ClearSendCallbacks();
	Native->ClearReceiveCallbacks();

	Super::EndPlay(EndPlayReason);
}

float FMediapipeTracking::readFloat(uint8_t* bytes, int& offset) {
	float f;
	uint8_t* f_ptr = (uint8_t*)&f;

	f_ptr[3] = bytes[3 + offset];
	f_ptr[2] = bytes[2 + offset];
	f_ptr[1] = bytes[1 + offset];
	f_ptr[0] = bytes[0 + offset];

	offset += 4;
	return f;
}

int FMediapipeTracking::readInt(uint8_t* bytes, int& offset) {
	int i;
	uint8_t* f_ptr = (uint8_t*)&i;

	f_ptr[3] = bytes[3 + offset];
	f_ptr[2] = bytes[2 + offset];
	f_ptr[1] = bytes[1 + offset];
	f_ptr[0] = bytes[0 + offset];

	offset += 4;

	return i;
}

FVector FMediapipeTracking::readVector3(uint8_t* bytes, int& offset)
{
	FVector vec3d;
	vec3d.Y = -readFloat(bytes, offset);
	vec3d.Z = -readFloat(bytes, offset);
	vec3d.X = -readFloat(bytes, offset);
	return vec3d;
}

void FMediapipeTracking::readFromPacket(uint8_t* b, int o) {
	
	int count = readInt(b, o);

	for (int i = 0; i < count; i++)
	{
		posePoints.Add(readVector3(b, o));
	}
	count = readInt(b, o);
	for (int i = 0; i < count; i++)
	{
		worldPosePoints.Add(readVector3(b, o));
	}
	count = readInt(b, o);
	for (int i = 0; i < count; i++)
	{
		leftHandPoints.Add(readVector3(b, o));
	}
	count = readInt(b, o);
	for (int i = 0; i < count; i++)
	{
		rightHandPoints.Add(readVector3(b, o));
	}
	count = readInt(b, o);
	for (int i = 0; i < count; i++)
	{
		facePoints.Add(readVector3(b, o));
	}
}


FMediapipeMessenger::FMediapipeMessenger()
{
	SenderSocket = nullptr;
	ReceiverSocket = nullptr;

	ClearReceiveCallbacks();
	ClearSendCallbacks();
}

FMediapipeMessenger::~FMediapipeMessenger()
{
	if (Settings.bIsReceiveOpen)
	{
		CloseReceiveSocket();
		ClearReceiveCallbacks();
	}
	if (Settings.bIsSendOpen)
	{
		CloseSendSocket();
		ClearSendCallbacks();
	}
}

int32 FMediapipeMessenger::OpenSendSocket(const FString& InIP /*= TEXT("127.0.0.1")*/, const int32 InPort /*= 3000*/)
{
	Settings.SendIP = InIP;
	Settings.SendPort = InPort;

	RemoteAdress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bIsValid;
	RemoteAdress->SetIp(*Settings.SendIP, bIsValid);
	RemoteAdress->SetPort(Settings.SendPort);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("UDP address is invalid <%s:%d>"), *Settings.SendIP, Settings.SendPort);
		return 0;
	}

	SenderSocket = FUdpSocketBuilder(*Settings.SendSocketName).AsReusable().WithBroadcast();

	//Set Send Buffer Size
	SenderSocket->SetSendBufferSize(Settings.BufferSize, Settings.BufferSize);
	SenderSocket->SetReceiveBufferSize(Settings.BufferSize, Settings.BufferSize);

	bool bDidConnect = SenderSocket->Connect(*RemoteAdress);
	Settings.bIsSendOpen = true;
	Settings.SendBoundPort = SenderSocket->GetPortNo();

	if (OnSendOpened)
	{
		OnSendOpened(Settings.SendPort, Settings.SendBoundPort);
	}

	return Settings.SendBoundPort;
}

bool FMediapipeMessenger::CloseSendSocket()
{
	bool bDidCloseCorrectly = true;
	Settings.bIsSendOpen = false;

	if (SenderSocket)
	{
		bDidCloseCorrectly = SenderSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SenderSocket);
		SenderSocket = nullptr;

		if (OnSendClosed)
		{
			OnSendClosed(Settings.SendPort);
		}
	}

	return bDidCloseCorrectly;
}

bool FMediapipeMessenger::EmitBytes(const TArray<uint8>& Bytes)
{
	bool bDidSendCorrectly = true;

	if (SenderSocket && SenderSocket->GetConnectionState() == SCS_Connected)
	{
		int32 BytesSent = 0;
		bDidSendCorrectly = SenderSocket->Send(Bytes.GetData(), Bytes.Num(), BytesSent);
	}
	else if (Settings.bShouldAutoOpenSend)
	{
		bool bDidOpen = OpenSendSocket(Settings.SendIP, Settings.SendPort) != 0;
		return bDidOpen && EmitBytes(Bytes);
	}

	return bDidSendCorrectly;
}

bool FMediapipeMessenger::OpenReceiveSocket(const FString& InListenIP /*= TEXT("0.0.0.0")*/, const int32 InListenPort /*= 3002*/)
{
	//Sync and overwrite settings
	if (Settings.bShouldOpenReceiveToBoundSendPort)
	{
		if (Settings.SendBoundPort == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("FUDPNative::OpenReceiveSocket Can't bind to SendBoundPort if send socket hasn't been opened before this call."));
			return false;
		}
		Settings.ReceiveIP = Settings.SendIP;
		Settings.ReceivePort = Settings.SendBoundPort;
	}
	else
	{
		Settings.ReceiveIP = InListenIP;
		Settings.ReceivePort = InListenPort;
	}

	bool bDidOpenCorrectly = true;

	if (Settings.bIsReceiveOpen)
	{
		bDidOpenCorrectly = CloseReceiveSocket();
	}

	FIPv4Address Addr;
	FIPv4Address::Parse(Settings.ReceiveIP, Addr);

	//Create Socket
	FIPv4Endpoint Endpoint(Addr, Settings.ReceivePort);

	ReceiverSocket = FUdpSocketBuilder(*Settings.ReceiveSocketName)
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(Settings.BufferSize);

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
	FString ThreadName = FString::Printf(TEXT("UDP RECEIVER-FUDPNative"));
	UDPReceiver = new FUdpSocketReceiver(ReceiverSocket, ThreadWaitTime, *ThreadName);

	UDPReceiver->OnDataReceived().BindLambda([this](const FArrayReaderPtr& DataPtr, const FIPv4Endpoint& Endpoint)
		{
			if (!OnReceivedBytes)
			{
				return;
			}

			TArray<uint8> Data;
			Data.AddUninitialized(DataPtr->TotalSize());
			DataPtr->Serialize(Data.GetData(), DataPtr->TotalSize());
			int length = Data.Num();
			if (length == 508)
			{

			}
			FMediapipeTracking mediapipeTracking;
			mediapipeTracking.readFromPacket(Data.GetData(), length- length);

			if (Settings.bReceiveDataOnGameThread)
			{
				//Copy data to receiving thread via lambda capture
				AsyncTask(ENamedThreads::GameThread, [this, mediapipeTracking]()
					{
						//double check we're still bound on this thread
						if (OnReceivedBytes)
						{
							OnReceivedBytes(mediapipeTracking);
						}
					});
			}
			else
			{
				OnReceivedBytes(mediapipeTracking);
			}
		});

	Settings.bIsReceiveOpen = true;

	if (OnReceiveOpened)
	{
		OnReceiveOpened(Settings.ReceivePort);
	}

	UDPReceiver->Start();

	return bDidOpenCorrectly;
}

bool FMediapipeMessenger::CloseReceiveSocket()
{
	bool bDidCloseCorrectly = true;
	Settings.bIsReceiveOpen = false;

	if (ReceiverSocket)
	{
		UDPReceiver->Stop();
		delete UDPReceiver;
		UDPReceiver = nullptr;

		bDidCloseCorrectly = ReceiverSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ReceiverSocket);
		ReceiverSocket = nullptr;

		if (OnReceiveClosed)
		{
			OnReceiveClosed(Settings.ReceivePort);
		}
	}

	return bDidCloseCorrectly;
}

void FMediapipeMessenger::ClearSendCallbacks()
{
	OnSendOpened = nullptr;
	OnSendClosed = nullptr;
}

void FMediapipeMessenger::ClearReceiveCallbacks()
{
	OnReceivedBytes = nullptr;
	OnReceiveOpened = nullptr;
	OnReceiveClosed = nullptr;
}

FMediapipeNetworkSettings::FMediapipeNetworkSettings()
{
	bShouldAutoOpenSend = false;
	bShouldAutoOpenReceive = true;
	bShouldOpenReceiveToBoundSendPort = false;
	bReceiveDataOnGameThread = true;
	SendIP = FString(TEXT("127.0.0.1"));
	SendPort = 3002;
	SendBoundPort = 0;	//invalid if 0
	ReceiveIP = FString(TEXT("127.0.0.1"));
	ReceivePort = 13037;
	SendSocketName = FString(TEXT("ue4-dgram-send"));
	ReceiveSocketName = FString(TEXT("ue4-dgram-receive"));

	bIsReceiveOpen = false;
	bIsSendOpen = false;

	BufferSize = 2 * 1024 * 1024;	//default roughly 2mb
}

