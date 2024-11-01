// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameInstanceSubsystem.h"
#include "WebSocketsModule.h"
#include <JsonObjectConverter.h>

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
	TArray<FString> ReceivedData =  ParsReceivedData(NewMessage);
	if (bool IsValidProtocol = CheckClientDataIsValid(ReceivedData); IsValidProtocol)
	{
		if (bDebug)
		{
			UE_LOG(LogTemp, Log, TEXT("Valid Data, WebSocket message received: %s"), *NewMessage);	
		}
		
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("The data is not valid please check it, data : %s"), *NewMessage);	
	}

}

void ULobbyGameInstanceSubsystem::OnClosed(int32 NewStatusCode, const FString& NewReason, bool NewWasClean)
{
	UE_LOG(LogTemp, Log, TEXT("WebSocket closed: %s"), *NewReason);	
}

TArray<FString> ULobbyGameInstanceSubsystem::ParsReceivedData(const FString& NewReceivedData) const
{
	TArray<FString> R_ParsedData{};
	R_ParsedData.Add(NewReceivedData.Left(32));  // Client ID

	int32 JsonStartIndex, BraceCount = 0, JsonEndIndex = -1;
	if (!NewReceivedData.FindChar('{', JsonStartIndex))
	{
		return R_ParsedData;
	}

	// Find the matching closing brace
	for (int32 i = JsonStartIndex; i < NewReceivedData.Len(); ++i)
	{
		BraceCount += (NewReceivedData[i] == '{') ? 1 : (NewReceivedData[i] == '}') ? -1 : 0;
		if (BraceCount == 0) { JsonEndIndex = i; break; }
	}
	if (JsonEndIndex == -1)
	{
		return R_ParsedData;
	}

	R_ParsedData.Add(NewReceivedData.Mid(32, JsonStartIndex - 32));  // Timestamp
	R_ParsedData.Add(NewReceivedData.Mid(JsonStartIndex, (JsonEndIndex - JsonStartIndex) + 1));  // JSON body
	R_ParsedData.Add(NewReceivedData.Mid(JsonEndIndex + 1));  // Signature
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

FString ULobbyGameInstanceSubsystem::GenerateRequestUniqueId() const
{
	// Generate a new GUID
	FGuid UniqueId = FGuid::NewGuid();
	// Convert the GUID to a string and return it
	FString R_RequestId = UniqueId.ToString(EGuidFormats::DigitsWithHyphens);
	return R_RequestId;
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


FString ULobbyGameInstanceSubsystem::SendData(const FNGGLobbyData& NewNGGLobbyData)
{
	if (WebSocket.IsValid())
	{
		FString JsonString{};
		FJsonObjectConverter::UStructToJsonObjectString<FNGGLobbyData>(NewNGGLobbyData, JsonString);
		UE_LOG(LogTemp, Log, TEXT("NewStringData = %s"), *JsonString);
		CookingDataAndSendToClient(JsonString);
	}
	return NewNGGLobbyData.requestId;
}

FString ULobbyGameInstanceSubsystem::SendChatMessage(const FChatData& NewChatData)
{
	FString JsonString{};
	FJsonObjectConverter::UStructToJsonObjectString<FChatData>(NewChatData, JsonString);
	FNGGLobbyData LobbyData{};
	LobbyData.Action = ELobbyActionType::TEXT_CHAT;
	LobbyData.ClientID = NewChatData.SenderPlayerId;
	LobbyData.PayLoadData = JsonString;
	LobbyData.requestId = GenerateRequestUniqueId();
	FString R_RequestId = SendData(LobbyData);
	return R_RequestId;
}

FString ULobbyGameInstanceSubsystem::SendDBRequest(const FMongoDBData& NewMongoDBdata)
{
	FString JsonString{};
	FJsonObjectConverter::UStructToJsonObjectString<FMongoDBData>(NewMongoDBdata, JsonString);
	FNGGLobbyData LobbyData{};
	LobbyData.Action = ELobbyActionType::DATABASE;
	LobbyData.ClientID = NewMongoDBdata.SenderPlayerId;
	LobbyData.PayLoadData = JsonString;
	LobbyData.requestId = GenerateRequestUniqueId();
	FString R_RequestId = SendData(LobbyData);
	return R_RequestId;
}

FString ULobbyGameInstanceSubsystem::RegisterPlayerIntoLobby(const FString& NewPlayerId)
{
	FNGGLobbyData LobbyData{};
	LobbyData.Action = ELobbyActionType::REGISTER_PLAYER_INTO_LOBBY;
	LobbyData.ClientID = NewPlayerId;
	LobbyData.PayLoadData = "";
	LobbyData.requestId = GenerateRequestUniqueId();
	FString R_RequestId =  SendData(LobbyData);
	return R_RequestId;
}

