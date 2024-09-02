// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IWebSocket.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LobbyGameInstanceSubsystem.generated.h"

// TODO need to move this information into Data Asset or ini file
USTRUCT(Blueprintable)
struct FJWTConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString ClientSecret = "4ab90ddc-caa3-42e0-ba9f-d985b9524660";

	UPROPERTY(BlueprintReadWrite)
	FString ClientId = "jGKD8Ceh1VOHCQdMlDtStIYHXDIdyCjE";

};


namespace NGG_LOBBY_PROTOCOL 
{
	enum PRATACOL
	{
		  CLIENT_ID = 0x0
		, TIMESTAMP = 0x1
		, JSON      = 0x2
		, SIGNATURE = 0x3
	};
	const int32 PART_PRATACOL_COUNT = 4;
};

UENUM(Blueprintable)
enum class ELobbyActionType : uint8
{
	 NONE    UMETA(DisplayName = "None")
	,DATABASE    UMETA(DisplayName = "Data Base")
	,TEXT_CHAT   UMETA(DisplayName = "Text Chat")
	,FIND_PLAYER UMETA(DisplayName = "Find Player")
};


USTRUCT(Blueprintable)
struct FNGGLobbyData
{
	GENERATED_BODY()

	/**
	* Client custom ID
	*/
	UPROPERTY(BlueprintReadWrite)
	FString ClientID = "";

	/**
	* The Lobby action type
	*/
	UPROPERTY(BlueprintReadWrite)
	ELobbyActionType Action = ELobbyActionType::NONE;

	/**
	* 
	* The PayLoadData can be JSON data
	*/
	UPROPERTY(BlueprintReadWrite)
	FString PayLoadData = "";

};

/**
 * 
 */
UCLASS(Blueprintable, Blueprinttype)
class LOBBYEXAMPLE_API ULobbyGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:

	/**
	* Web socket 
	*/
	TSharedPtr<IWebSocket> WebSocket;
	
	/**
	* bDebug if ture then write log	
	*/
	uint8 bDebug : 1;

	/**
	*	JWET Config TODO the JWTConfig must read from DataAsset	
	*/
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess=true))
	FJWTConfig JWTConfig;

public:

	
	/**
	 * construct  
	 */
	ULobbyGameInstanceSubsystem();

	~ULobbyGameInstanceSubsystem();

private:



	
	void OnConnected();

	void OnConnectionError(const FString& NewError);

	void OnMessageReceived(const FString& NewMessage);

	void OnClosed(int32 NewStatusCode, const FString& NewReason, bool NewWasClean);

private:

	FString GenerateSignature(const FString& NewClientSecret, const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp) const;

	FString MakeSendData(const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp, const FString& NewSignature = "") const;

	TArray<FString> ParsReceivedData(const FString & NewReceivedData) const;

	bool IsValidSignature(const TArray<FString>& NewReceivedData, const FString& NewReceivedSignature) const;

	FString GetClientSecret() const;

	FString GetClientId() const;

	void CookingDataAndSendToClient(const FString& NewData);
	
	bool CheckClientDataIsValid(const TArray<FString>& NewClientDataStringArray);

	int64 GetTimestamp() const;

	
public:
	
	UFUNCTION(BlueprintCallable)
	void ConnectToLobbyServer(const FString & NewURL);

	UFUNCTION(BlueprintCallable)
	void SendMessage(FString NewStringData);

	UFUNCTION(BlueprintCallable)
	void SendData(const FNGGLobbyData & NewNGGLobbyData);

	
	
	
};
