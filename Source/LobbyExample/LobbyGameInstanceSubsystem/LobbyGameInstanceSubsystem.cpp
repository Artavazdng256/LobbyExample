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

FString ULobbyGameInstanceSubsystem::GenerateSignature(const FString& NewClientSecret, const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp) const
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

FString ULobbyGameInstanceSubsystem::MakeSendData(const FString& NewRequestBodyString, const FString& NewClientId, int64 NewTimestamp, const FString& NewSignature) const
{
	FString Body = NewRequestBodyString;
	// Concatenate ClientID, Timestamp, and Body
	FString MessageStr = NewClientId + FString::Printf(TEXT("%lld"), NewTimestamp) + Body + NewSignature;
	// Debug output
	if (bDebug)
	{
		UE_LOG(LogTemp, Log, TEXT("----------------- Message ----------------------"));
		UE_LOG(LogTemp, Log, TEXT("%s"), *MessageStr);
		UE_LOG(LogTemp, Log, TEXT("----------------- Message ----------------------"));
	}
	return MessageStr;
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

TArray<FString> ULobbyGameInstanceSubsystem::ParsReceivedData(const FString& NewReceivedData) const
{
	TArray<FString> R_ParsedData{};
    FString ClientID = NewReceivedData.Left(32);
    R_ParsedData.Add(ClientID);

    int32 JsonStartIndex;
    int32 JsonEndIndex;

    if (!NewReceivedData.FindChar('{', JsonStartIndex) || !NewReceivedData.FindChar('}', JsonEndIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("ParseReceivedData function Error: JSON body not found."));
        return R_ParsedData;
    }

    FString Timestamp = NewReceivedData.Mid(32, JsonStartIndex - 32);
    R_ParsedData.Add(Timestamp);

    FString JsonBody = NewReceivedData.Mid(JsonStartIndex, (JsonEndIndex - JsonStartIndex) + 1);
    R_ParsedData.Add(JsonBody);

    FString Signature = NewReceivedData.Mid(JsonEndIndex + 1);
    R_ParsedData.Add(Signature);
	
	return R_ParsedData;
}

FString ULobbyGameInstanceSubsystem::GetClientSecret() const
{
	return JWTConfig.ClientSecret;
}

FString ULobbyGameInstanceSubsystem::GetClientId() const
{
	return JWTConfig.ClientId;
}

void ULobbyGameInstanceSubsystem::CookingDataAndSendToClient(const FString& NewData)
{
	if (WebSocket.IsValid())
	{
		FString ClientId = GetClientId();
		FString ClientSecret = GetClientSecret();
		int64 Timestamp = GetTimestamp();

		FString DataWithoutSignature = MakeSendData(NewData, ClientId, Timestamp);
		FString Signature = GenerateSignature(ClientSecret, NewData, ClientId, Timestamp);
		FString CookedData = MakeSendData(NewData, ClientId, Timestamp, Signature);

		WebSocket->Send(CookedData);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Function CookingDataAndSendToClient: The Client is nullptr."));
	}
}

bool ULobbyGameInstanceSubsystem::CheckClientDataIsValid(const TArray<FString>& NewClientDataStringArray)
{
	bool bStatus = false;

	if (WebSocket.IsValid())
	{
		int32 SplitedDataCount = NewClientDataStringArray.Num();
		if (NGG_LOBBY_PROTOCOL::PART_PRATACOL_COUNT == SplitedDataCount)
		{
			FString ClientSignature = NewClientDataStringArray[NGG_LOBBY_PROTOCOL::SIGNATURE];
			bStatus = IsValidSignature(NewClientDataStringArray, ClientSignature);
		}
		else
		{
			// Disconnect the client because the data protocol is invalid.
			WebSocket->Close();
		}
	}

	return bStatus;
}

int64 ULobbyGameInstanceSubsystem::GetTimestamp() const
{
	FDateTime CurrentDateTime = FDateTime::UtcNow();
    int64 Timestamp = CurrentDateTime.ToUnixTimestamp() * 1000 + CurrentDateTime.GetMillisecond();
    return Timestamp;	
}


bool ULobbyGameInstanceSubsystem::IsValidSignature(const TArray<FString>& NewReceivedData, const FString& NewReceivedSignature) const
{
	const int DataCount = NewReceivedData.Num();
	if (NGG_LOBBY_PROTOCOL::PART_PRATACOL_COUNT == DataCount)
	{
		FString ServerSideClientID = GetClientId();
		FString ServerSideClientSecret = GetClientSecret();

		int64 Timestamp = FCString::Atoi64(*NewReceivedData[NGG_LOBBY_PROTOCOL::TIMESTAMP]);
		FString Body = NewReceivedData[NGG_LOBBY_PROTOCOL::JSON];

		FString ServerSideSignature = GenerateSignature(ServerSideClientSecret, Body, ServerSideClientID, Timestamp);

		FString ServerSideSignatureLower = ServerSideSignature.ToLower();
		FString ClientSignatureLower = NewReceivedSignature.ToLower();

		if (ClientSignatureLower == ServerSideSignatureLower)
		{
			return true;
		}
	}
	else
	{
		// TODO: Disconnect the client if the received data is not valid.
		UE_LOG(LogTemp, Warning, TEXT("The ReceivedData is not valid. Please check."));
	}
	return false;
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

