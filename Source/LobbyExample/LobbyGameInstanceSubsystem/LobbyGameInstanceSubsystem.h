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
	
public:
	
	UFUNCTION(BlueprintCallable)
	void ConnectToLobbyServer(const FString & NewURL);

	UFUNCTION(BlueprintCallable)
	void SendMessage(FString NewMessage);

	
	
	
};
