// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameInstanceSubsystem.h"
#include "WebSocketsModule.h"

ULobbyGameInstanceSubsystem::ULobbyGameInstanceSubsystem()
{
	// Initialize the WebSocket module
	FModuleManager::Get().LoadModuleChecked<FWebSocketsModule>("WebSockets");	
}

ULobbyGameInstanceSubsystem::~ULobbyGameInstanceSubsystem()
{
	if(WebSocket.IsValid())
	{
		WebSocket->Close();
	}
}

void ULobbyGameInstanceSubsystem::OnConnected()
{
	UE_LOG(LogTemp, Log, TEXT("WebSocket connected!"));
}

void ULobbyGameInstanceSubsystem::OnConnectionError(const FString& NewError)
{
	UE_LOG(LogTemp, Error, TEXT("WebSocket connection error: %s"), *NewError);	
}

void ULobbyGameInstanceSubsystem::OnMessageReceived(const FString& NewMessage)
{
	UE_LOG(LogTemp, Log, TEXT("WebSocket message received: %s"), *NewMessage);	
}

void ULobbyGameInstanceSubsystem::OnClosed(int32 NewStatusCode, const FString& NewReason, bool NewWasClean)
{
	UE_LOG(LogTemp, Log, TEXT("WebSocket closed: %s"), *NewReason);	
}


void ULobbyGameInstanceSubsystem::ConnectToLobbyServer(const FString& NewURL)
{
	WebSocket = FWebSocketsModule::Get().CreateWebSocket(NewURL, "wss");
	if(WebSocket.IsValid())
	{
		WebSocket->OnConnected().AddUObject(this, &ULobbyGameInstanceSubsystem::OnConnected);
		WebSocket->OnConnectionError().AddUObject(this, &ULobbyGameInstanceSubsystem::OnConnectionError);
		WebSocket->OnMessage().AddUObject(this, &ULobbyGameInstanceSubsystem::OnMessageReceived);
		WebSocket->OnClosed().AddUObject(this, &ULobbyGameInstanceSubsystem::OnClosed);
		WebSocket->Connect();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("The WebSocket is nullptr. Please check WebSocket pointer"));
	}


	
}

void ULobbyGameInstanceSubsystem::SendMessage(FString NewMessage)
{
	if(WebSocket.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("NewMessage = %s"), *NewMessage);
		WebSocket->Send(NewMessage);
	}
}

