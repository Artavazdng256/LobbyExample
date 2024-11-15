﻿// Fill out your copyright notice in the Description page of Project Settings.

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
	 NONE						UMETA(DisplayName = "None")
	,DATABASE					UMETA(DisplayName = "Data Base")
	,TEXT_CHAT					UMETA(DisplayName = "Text Chat")
	,FIND_PLAYER				UMETA(DisplayName = "Find Player")
	,REGISTER_PLAYER_INTO_LOBBY UMETA(DisplayName = "Register Player")
	,REQUEST_STATUS 			UMETA(DisplayName = "Request Status")
};

UENUM(Blueprintable)
enum class EMongoDBActionType : uint8
{
	NONE							UMETA(DisplayName = "None")
	,DROP_COLLECTION				UMETA(DisplayName = "Drop Collection")
	,CREATE_COLLECTION				UMETA(DisplayName = "Create Collection")
    ,FIND							UMETA(DisplayName = "Find")
    ,FIND_WITH_OPTIONS				UMETA(DisplayName = "Find With Options")
    ,FIND_ONE						UMETA(DisplayName = "Find One")
	,FIND_ONE_WITH_OPTIONS			UMETA(DisplayName = "Find One With Options")
	,INSERT_ONE						UMETA(DisplayName = "Insert One")
	,INSERT_MANY					UMETA(DisplayName = "Insert Many")
	,LIST_DATABASES					UMETA(DisplayName = "List Databases")
	,LIST_COLLECTION_NAMES			UMETA(DisplayName = "List Collection Names")
	,LIST_INDEXES					UMETA(DisplayName = "List Indexes")
	,CREATE_INDEX					UMETA(DisplayName = "Create Index")
	,DELETE_ONE						UMETA(DisplayName = "Delete One")
    ,DELETE_MANY					UMETA(DisplayName = "Delete Many")
    ,GET_ESTIMATED_DOCUMENT_COUNT	UMETA(DisplayName = "Get Estimated Document Count")	
    ,COUNT_DOCUMENTS 				UMETA(DisplayName = "Count Documents")
    ,RENAME_COLLECTION              UMETA(DisplayName = "Rename Collection")
	,RUN_COMMAND					UMETA(DisplayName = "Run Command")
	,REPLACE_ONE					UMETA(DisplayName = "Replace One")
	,UPDATE_ONE						UMETA(DisplayName = "Update One")
	,UPDATE_ONE_WITH_OPTIONS 		UMETA(DisplayName = "Update One With Options")
	,UPDATE_MANY			 		UMETA(DisplayName = "Update Many")
	,UPDATE_MANY_WITH_OPTIONS 		UMETA(DisplayName = "Update Many with Options")
	,FIND_AND_DELETE 				UMETA(DisplayName = "Find and delete")
	,FIND_AND_REPLACE 				UMETA(DisplayName = "Find and Replace")
	,FIND_AND_UPDATE 				UMETA(DisplayName = "Find and Update")
};


USTRUCT(BlueprintType, Blueprintable)
struct FNGGLobbyData 
{
	GENERATED_BODY()

	/**
	* Client custom ID
	*/
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "ClientID"))
	FString ClientID = "";

	/**
	* The Lobby action type
	*/
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "Action"))
	ELobbyActionType Action = ELobbyActionType::NONE;

	/**
	* 
	* The PayLoadData can be JSON data
	*/
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "PayLoadData"))
	FString PayLoadData = "";

	/**
	 *	The Request id  
	 */
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "RequestId"))
	FString requestId = "";


};

USTRUCT(BlueprintType, Blueprintable)
struct FChatData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "SenderPlayerId"))
	FString SenderPlayerId = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "Message"))
	FString Message = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "RecipientPlayerId"))
	FString RecipientPlayerId = "";
};

USTRUCT(BlueprintType, Blueprintable)
struct FMongoDBData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "SenderPlayerId"))
	FString SenderPlayerId = "";
	
	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "DbName"))
	FString DbName = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "CollectionName"))
	FString CollectionName = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "DbAction"))
	EMongoDBActionType DbAction = EMongoDBActionType::NONE;

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "Data"))
	FString Data = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "Filter"))
	FString Filter = "";

	UPROPERTY(BlueprintReadWrite, Meta = (DisplayName = "Options"))
	FString Options = "";
};

/**
 * 
 */
UCLASS(Blueprintable, Blueprinttype)
class LOBBYCLIENT_API ULobbyGameInstanceSubsystem : public UGameInstanceSubsystem
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

	FString GenerateRequestUniqueId() const;

	
	UFUNCTION(BlueprintCallable)
	void ConnectToLobbyServer(const FString & NewURL);

	UFUNCTION(BlueprintCallable)
	FString SendData(const FNGGLobbyData& NewNGGLobbyData);

	UFUNCTION(BlueprintCallable)
	FString SendChatMessage(const FChatData& NewChatData);

	UFUNCTION(BlueprintCallable)
	FString SendDBRequest(const FMongoDBData& NewMongoDBdata);

	UFUNCTION(BlueprintCallable)
	FString RegisterPlayerIntoLobby(const FString& NewPlayerId);
	
	
	
};
