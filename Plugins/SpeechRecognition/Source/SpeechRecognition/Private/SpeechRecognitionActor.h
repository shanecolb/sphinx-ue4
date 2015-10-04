
#pragma once

#include "SpeechRecognitionWorker.h"
#include "SpeechRecognitionActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWordsSpokenSignature, FString, Text);

UCLASS(BlueprintType, Blueprintable)
class SPEECHRECOGNITION_API ASpeechRecognitionActor : public AActor
{
	GENERATED_BODY()

private:
	
	FSpeechRecognitionWorker* listenerThread;

public:

	// Basic functions 
	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (FriendlyName = "Init", Keywords = "Speech Recognition Init"))
	bool Init(ESpeechRecognitionLanguage language);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (FriendlyName = "Shutdown", Keywords = "Speech Recognition Shutdown"))
	void Shutdown();

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (FriendlyName = "Add Words", Keywords = "Speech Recognition"))
	void AddWords(TArray<FString> wordList);

	UFUNCTION()
	void WordSpoken_method(FString text);

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Audio")
	FWordsSpokenSignature OnWordSpoken;
};

