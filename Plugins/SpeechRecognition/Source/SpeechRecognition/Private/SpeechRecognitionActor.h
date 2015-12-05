
#pragma once

#include "SpeechRecognitionWorker.h"
#include "TaskGraphInterfaces.h"
#include "SpeechRecognitionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWordsSpokenSignature, FString, Text);

UCLASS(BlueprintType, Blueprintable)
class SPEECHRECOGNITION_API ASpeechRecognitionActor : public AActor
{
	GENERATED_BODY()

private:

	int32 instanceCtr;
	
	FSpeechRecognitionWorker* listenerThread;

	static void WordSpoken_trigger(FWordsSpokenSignature delegate_method, FString text);

public:

	// Basic functions 
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (FriendlyName = "Init", Keywords = "Speech Recognition Init"))
	bool Init(ESpeechRecognitionLanguage language, TArray<FRecognitionKeyWord> wordList);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (FriendlyName = "Shutdown", Keywords = "Speech Recognition Shutdown"))
	bool Shutdown();

	UFUNCTION()
	void WordSpoken_method(FString text);

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FWordsSpokenSignature OnWordSpoken;

};
