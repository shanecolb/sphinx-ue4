#include "SpeechRecognition.h"
#include "SpeechRecognitionActor.h"

#define SPEECHRECOGNITIONPLUGIN ISpeechRecognition::Get()

bool ASpeechRecognitionActor::Init(ESpeechRecognitionLanguage language, TArray<FRecognitionPhrase> wordList)
{
	// terminate any existing thread
	if (listenerThread != NULL)
		Shutdown();

	// start listener thread
	listenerThread = new FSpeechRecognitionWorker();
	TArray<FRecognitionPhrase> dictionaryList;
	listenerThread->SetLanguage(language);
	listenerThread->AddWords(wordList);
	bool threadSuccess = listenerThread->StartThread(this);
	return threadSuccess;
}

bool ASpeechRecognitionActor::Shutdown()
{
	if (listenerThread != NULL) {
		listenerThread->ShutDown();
		listenerThread = NULL;
		return true;
	}
	else{
		return false;
	}
}

void ASpeechRecognitionActor::WordSpoken_trigger(FWordsSpokenSignature delegate_method, FString text)
{
	delegate_method.Broadcast(text);
}

void ASpeechRecognitionActor::WordSpoken_method(FString text)
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
		FSimpleDelegateGraphTask::FDelegate::CreateStatic(&WordSpoken_trigger, OnWordSpoken, text)
		, TStatId()
		, nullptr
		, ENamedThreads::GameThread
		);


}

void ASpeechRecognitionActor::StartedSpeaking_trigger(FStartedSpeakingSignature delegate_method)
{
	delegate_method.Broadcast();
}

void ASpeechRecognitionActor::StartedSpeaking_method()
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
		FSimpleDelegateGraphTask::FDelegate::CreateStatic(&StartedSpeaking_trigger, OnStartedSpeaking)
		, TStatId()
		, nullptr
		, ENamedThreads::GameThread
		);


}

void ASpeechRecognitionActor::StoppedSpeaking_trigger(FStoppedSpeakingSignature delegate_method)
{
	delegate_method.Broadcast();
}

void ASpeechRecognitionActor::StoppedSpeaking_method()
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
		FSimpleDelegateGraphTask::FDelegate::CreateStatic(&StoppedSpeaking_trigger, OnStoppedSpeaking)
		, TStatId()
		, nullptr
		, ENamedThreads::GameThread
		);


}