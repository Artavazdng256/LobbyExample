// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "IWebSocket.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LobbyGameInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class LOBBYEXAMPLE_API ULobbyGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

private:

	
	TSharedPtr<IWebSocket> WebSocket;

	uint8 bDebug : 1;

public:

	
	/**
	 * construct  
	 */
	ULobbyGameInstanceSubsystem();

	~ULobbyGameInstanceSubsystem();

private:


	FString GenerateSignature(const FString& NewClientSecret, const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp);

	
	void OnConnected();

	void OnConnectionError(const FString& NewError);

	void OnMessageReceived(const FString& NewMessage);

	void OnClosed(int32 NewStatusCode, const FString& NewReason, bool NewWasClean);
	
public:
	
	UFUNCTION(BlueprintCallable)
	void ConnectToLobbyServer(const FString & NewURL);

	UFUNCTION(BlueprintCallable)
	void SendMessage(FString NewMessage);

	
	
	
};
