#pragma once

#include "Engine.h"
#include "Kismet/GameplayStatics.h"
#include "ISpeechRecognition.h"
#include "SpeechRecognition.generated.h"

//Common structures and enumerations
USTRUCT(BlueprintType)
struct FRecognisedPhrases
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> phrases;

	// default constructor
	FRecognisedPhrases() {
	}
};

UENUM(BlueprintType)
enum class ESpeechRecognitionMode : uint8
{
	VE_KEYWORD 	UMETA(DisplayName = "Keyword Spotting"),
	VE_GRAMMAR  UMETA(DisplayName = "Grammar")
};

UENUM(BlueprintType)
enum class ESpeechRecognitionParamType : uint8
{
	VE_FLOAT 	UMETA(DisplayName = "Float"),
	VE_BOOLEAN  UMETA(DisplayName = "Boolean"),
	VE_STRING   UMETA(DisplayName = "String"),
	VE_INTEGER  UMETA(DisplayName = "Integer")
};

UENUM(BlueprintType)
enum class ESpeechRecognitionLanguage : uint8
{
	VE_English 	UMETA(DisplayName = "English"),
	VE_Chinese  UMETA(DisplayName = "Chinese"),
	VE_French	UMETA(DisplayName = "French")
};

UENUM(BlueprintType)
enum class EPhraseRecognitionTolerance : uint8
{
	VE_1 	UMETA(DisplayName = "V1"),
	VE_2 	UMETA(DisplayName = "V2"),
	VE_3 	UMETA(DisplayName = "V3"),
	VE_4 	UMETA(DisplayName = "V4"),
	VE_5 	UMETA(DisplayName = "V5"),
	VE_6 	UMETA(DisplayName = "V6"),
	VE_7 	UMETA(DisplayName = "V7"),
	VE_8 	UMETA(DisplayName = "V8"),
	VE_9 	UMETA(DisplayName = "V9"),
	VE_10 	UMETA(DisplayName = "V10")
};

USTRUCT(BlueprintType)
struct FRecognitionPhrase
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY(BlueprintReadWrite)
		FString phrase;

	UPROPERTY(BlueprintReadWrite)
		EPhraseRecognitionTolerance tolerance;

	// default constructor
	FRecognitionPhrase() {
	}

	// if you wish to only provide a phrase
	FRecognitionPhrase(FString phrase) {
		this->phrase = phrase;
		tolerance = EPhraseRecognitionTolerance::VE_5;
	}

	// if you wish to specify both a phrase, and a tolerance setting
	FRecognitionPhrase(FString phrase, EPhraseRecognitionTolerance tolerance) {
		this->phrase = phrase;
		this->tolerance = tolerance;
	}
};

class FSpeechRecognition :public ISpeechRecognition
{

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Search for a dll to be loaded dynamically */
	bool SearchForDllPath(FString _searchBase, FString _dllName);

};
