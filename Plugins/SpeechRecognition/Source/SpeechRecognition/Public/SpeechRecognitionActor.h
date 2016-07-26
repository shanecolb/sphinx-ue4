
#pragma once

#include "SpeechRecognitionWorker.h"
#include "SpeechRecognition.h"
#include "TaskGraphInterfaces.h"
#include "SpeechRecognitionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartedSpeakingSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStoppedSpeakingSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWordsSpokenSignature, FRecognisedPhrases, Text);

UCLASS(BlueprintType, Blueprintable)
class SPEECHRECOGNITION_API ASpeechRecognitionActor : public AActor
{
	GENERATED_BODY()

private:

	int32 instanceCtr;
	
	FSpeechRecognitionWorker* listenerThread;

	static void WordsSpoken_trigger(FWordsSpokenSignature delegate_method, FRecognisedPhrases text);
	static void StartedSpeaking_trigger(FStartedSpeakingSignature delegate_method);
	static void StoppedSpeaking_trigger(FStoppedSpeakingSignature delegate_method);

public:

	//Methods to switch recognition modes
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Enable Keyword Mode", Keywords = "Speech Recognition Mode"))
	bool EnableKeywordMode(TArray<FRecognitionPhrase> wordList);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Enable Grammar Mode", Keywords = "Speech Recognition Mode"))
	bool EnableGrammarMode(FString grammarName);

	// Basic functions 
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Init", Keywords = "Speech Recognition Init"))
	bool Init(ESpeechRecognitionLanguage language);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "SetConfigParam", Keywords = "Speech Recognition Set Config Param"))
	bool SetConfigParam(FString param, ESpeechRecognitionParamType type, FString value);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Shutdown", Keywords = "Speech Recognition Shutdown"))
	bool Shutdown();

	// Callback events
	UFUNCTION()
	void WordsSpoken_method(FRecognisedPhrases phrases);

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FWordsSpokenSignature OnWordsSpoken;

	UFUNCTION()
	void StartedSpeaking_method();

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FStartedSpeakingSignature OnStartedSpeaking;

	UFUNCTION()
	void StoppedSpeaking_method();

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FStoppedSpeakingSignature OnStoppedSpeaking;

};
