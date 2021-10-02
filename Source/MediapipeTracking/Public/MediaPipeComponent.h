// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sockets/Public/IPAddress.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketSender.h"
#include "Containers/List.h"
#include "MediaPipeComponent.generated.h"

USTRUCT(BlueprintType)
struct MEDIAPIPETRACKING_API FMediapipeTracking
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
	TArray<FVector> leftHandPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
	TArray<FVector> rightHandPoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
	TArray<FVector> posePoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
	TArray<FVector> worldPosePoints;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
	TArray<FVector> facePoints;
	float readFloat(uint8_t* bytes, int& offset);
	int readInt(uint8_t* bytes, int& offset);
	FVector readVector3(uint8_t* bytes, int& offset);
	void readFromPacket(uint8_t* b, int o);
};

USTRUCT(BlueprintType)
struct MEDIAPIPETRACKING_API FMediapipeNetworkSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		FString SendIP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		int32 SendPort;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		int32 SendBoundPort;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		FString ReceiveIP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		int32 ReceivePort;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		FString SendSocketName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		FString ReceiveSocketName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		int32 BufferSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		bool bShouldAutoOpenSend;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		bool bShouldAutoOpenReceive;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		bool bShouldOpenReceiveToBoundSendPort;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		bool bReceiveDataOnGameThread;
	UPROPERTY(BlueprintReadOnly, Category = "Mediapipe")
		bool bIsReceiveOpen;
	UPROPERTY(BlueprintReadOnly, Category = "Mediapipe")
		bool bIsSendOpen;
	FMediapipeNetworkSettings();
};

class MEDIAPIPETRACKING_API FMediapipeMessenger
{
public:

	TFunction<void(FMediapipeTracking)> OnReceivedBytes;
	TFunction<void(int32 Port)> OnReceiveOpened;
	TFunction<void(int32 Port)> OnReceiveClosed;
	TFunction<void(int32 SpecifiedPort, int32 BoundPort)> OnSendOpened;
	TFunction<void(int32 Port)> OnSendClosed;
	FMediapipeNetworkSettings Settings;

	FMediapipeMessenger();
	~FMediapipeMessenger();

	int32 OpenSendSocket(const FString& InIP = TEXT("127.0.0.1"), const int32 InPort = 3000);
	bool CloseSendSocket();
	
	bool EmitBytes(const TArray<uint8>& Bytes);
	bool OpenReceiveSocket(const FString& InIP = TEXT("127.0.0.1"), const int32 InListenPort = 13037);
	bool CloseReceiveSocket();
	void ClearSendCallbacks();
	void ClearReceiveCallbacks();

protected:
	FSocket* SenderSocket;
	FSocket* ReceiverSocket;
	FUdpSocketReceiver* UDPReceiver;
	FString SocketDescription;
	TSharedPtr<FInternetAddr> RemoteAdress;
	ISocketSubsystem* SocketSubsystem;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMediapipeSocketStateSig, int32, Port);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMediapipeSocketSendStateSig, int32, SpecifiedPort, int32, BoundPort);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMediapipeMessageSig, FMediapipeTracking, TrackingData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MEDIAPIPETRACKING_API UMediaPipeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMediaPipeComponent();

	UPROPERTY(BlueprintAssignable, Category = "Mediapipe")
		FMediapipeMessageSig OnReceivedData;

	/** Callback when we start listening on the udp receive socket*/
	UPROPERTY(BlueprintAssignable, Category = "Mediapipe")
		FMediapipeSocketStateSig OnReceiveSocketOpened;

	/** Called after receiving socket has been closed. */
	UPROPERTY(BlueprintAssignable, Category = "Mediapipe")
		FMediapipeSocketStateSig OnReceiveSocketClosed;

	/** Called when the send socket is ready to use; optionally open your receive socket to bound send port here */
	UPROPERTY(BlueprintAssignable, Category = "Mediapipe")
		FMediapipeSocketSendStateSig OnSendSocketOpened;

	/** Called when the send socket has been closed */
	UPROPERTY(BlueprintAssignable, Category = "Mediapipe")
		FMediapipeSocketStateSig OnSendSocketClosed;

	/** Defining UDP sending and receiving Ips, ports, and other defaults*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mediapipe")
		FMediapipeNetworkSettings Settings;

	UFUNCTION(BlueprintCallable, Category = "Mediapipe")
		int32 OpenSendSocket(const FString& InIP = TEXT("127.0.0.1"), const int32 InPort = 3000);

	/**
	* Close the sending socket. This is usually automatically done on EndPlay.
	*/
	UFUNCTION(BlueprintCallable, Category = "Mediapipe")
		bool CloseSendSocket();

	/**
	* Start listening at given port for udp messages. Will auto-listen on BeginPlay by default. Listen IP of 0.0.0.0 means all connections.
	*/
	UFUNCTION(BlueprintCallable, Category = "Mediapipe")
		bool OpenReceiveSocket(const FString& InListenIP = TEXT("127.0.0.1"), const int32 InListenPort = 13037);

	/**
	* Close the receiving socket. This is usually automatically done on EndPlay.
	*/
	UFUNCTION(BlueprintCallable, Category = "Mediapipe")
		bool CloseReceiveSocket();

	/**
	* Emit specified bytes to the udp channel.
	*
	* @param Message	Bytes
	*/
	UFUNCTION(BlueprintCallable, Category = "Mediapipe")
		bool EmitBytes(const TArray<uint8>& Bytes);

protected:
	TSharedPtr<FMediapipeMessenger> Native;

	void LinkupCallbacks();
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
