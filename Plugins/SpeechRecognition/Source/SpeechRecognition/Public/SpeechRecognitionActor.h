
#pragma once

#include "SpeechRecognitionWorker.h"
#include "SpeechRecognition.h"
#include "TaskGraphInterfaces.h"
#include "SpeechRecognitionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartedSpeakingSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStoppedSpeakingSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWordsSpokenSignature, FRecognisedPhrases, phrases);

UCLASS(BlueprintType, Blueprintable)
class SPEECHRECOGNITION_API ASpeechRecognitionActor : public AActor
{
	GENERATED_BODY()

private:

	int32 instanceCtr;
	
	FSpeechRecognitionWorker* listenerThread;
	bool live_recognition;
	float delay_time;

	static void WordsSpoken_trigger(FWordsSpokenSignature delegate_method, FRecognisedPhrases text);
	static void StartedSpeaking_trigger(FStartedSpeakingSignature delegate_method);
	static void StoppedSpeaking_trigger(FStoppedSpeakingSignature delegate_method);

public:
	/** Live recognition will process words as they are heard. 
Otherwise speech will be processed when speaking ends. */
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetLiveRecognition(bool live_recognition);

	bool GetLiveRecognition();
	float GetDelayTime();

	//Methods to switch recognition modes
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Enable Keyword Mode", Keywords = "Speech Recognition Mode"))
	bool EnableKeywordMode(TArray<FRecognitionPhrase> wordList);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Enable Grammar Mode", Keywords = "Speech Recognition Mode"))
	bool EnableGrammarMode(FString grammarName);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Enable Phonetic Mode", Keywords = "Speech Recognition Mode"))
	bool EnablePhoneticMode();

	// Basic functions 
	/** Live recognition will process words as they are heard.
Otherwise speech will be processed when speaking ends.
Delay Time is only used for Live Recognition.*/
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (DisplayName = "Init", Keywords = "Speech Recognition Init"))
	bool Init(ESpeechRecognitionLanguage language, bool live_recognition, float delay_time = 0.1);

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
