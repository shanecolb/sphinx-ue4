
#pragma once

#include "SpeechRecognitionWorker.h"
#include "TaskGraphInterfaces.h"
#include "SpeechRecognitionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartedSpeakingSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStoppedSpeakingSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWordsSpokenSignature, FString, Text);

UCLASS(BlueprintType, Blueprintable)
class SPEECHRECOGNITION_API ASpeechRecognitionActor : public AActor
{
	GENERATED_BODY()

private:

	int32 instanceCtr;
	
	FSpeechRecognitionWorker* listenerThread;

	static void WordSpoken_trigger(FWordsSpokenSignature delegate_method, FString text);
	static void StartedSpeaking_trigger(FStartedSpeakingSignature delegate_method);
	static void StoppedSpeaking_trigger(FStoppedSpeakingSignature delegate_method);

public:

	// Basic functions 
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Init", Keywords = "Speech Recognition Init"))
	bool Init(ESpeechRecognitionLanguage language, TArray<FRecognitionPhrase> wordList);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Shutdown", Keywords = "Speech Recognition Shutdown"))
	bool Shutdown();

	UFUNCTION()
	void WordSpoken_method(FString text);

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FWordsSpokenSignature OnWordSpoken;

	UFUNCTION()
	void StartedSpeaking_method();

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FStartedSpeakingSignature OnStartedSpeaking;

	UFUNCTION()
	void StoppedSpeaking_method();

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FStoppedSpeakingSignature OnStoppedSpeaking;
};
