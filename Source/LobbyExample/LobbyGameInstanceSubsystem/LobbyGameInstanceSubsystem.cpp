// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameInstanceSubsystem.h"
#include "WebSocketsModule.h"

namespace OpenSSLWrapper
{
#include "openssl/hmac.h"
#include "openssl/evp.h"
#include "Misc/Guid.h"
}


ULobbyGameInstanceSubsystem::ULobbyGameInstanceSubsystem()
	: bDebug(true)
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

FString ULobbyGameInstanceSubsystem::GenerateSignature(const FString& NewClientSecret, const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp)
{
	// Use FString instead of std::string
	FString SecretStr = NewClientSecret;
	FString Body = NewRequestBodyString;

	FString MessageStr = NewClientId + FString::Printf(TEXT("%lld"), NewTimestamp) + Body;
	if (bDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("----------------- Message ----------------------"));
		UE_LOG(LogTemp, Log, TEXT("%s"), *MessageStr);
		UE_LOG(LogTemp, Log, TEXT("----------------- Message ----------------------"));
	}

	// HMAC-SHA256 computation
	unsigned char Result[64];
	unsigned int ResultLength;

	// Convert FString to UTF-8 encoded char* for HMAC computation
	FTCHARToUTF8 SecretUtf8(*SecretStr);
	FTCHARToUTF8 MessageUtf8(*MessageStr);

	OpenSSLWrapper::HMAC(OpenSSLWrapper::EVP_sha256(), SecretUtf8.Get(), SecretUtf8.Length(),
		reinterpret_cast<const unsigned char*>(MessageUtf8.Get()), MessageUtf8.Length(),
		Result, &ResultLength);

	// Convert result to hex string
	FString Signature = BytesToHex(Result, ResultLength);
	if (bDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("Signature : %s"), *Signature);
	}
	return Signature.ToLower();
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

