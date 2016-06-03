#include "SpeechRecognition.h"
#include "SpeechRecognitionActor.h"

#define SPEECHRECOGNITIONPLUGIN ISpeechRecognition::Get()

bool ASpeechRecognitionActor::Init(ESpeechRecognitionLanguage language, bool live_recognition, float delay_time)
{
	this->live_recognition = live_recognition;
	this->delay_time = delay_time;
	// terminate any existing thread
	if (listenerThread != NULL)
		Shutdown();

	// start listener thread
	listenerThread = new FSpeechRecognitionWorker();
	TArray<FRecognitionPhrase> dictionaryList;
	listenerThread->SetLanguage(language);
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


void ASpeechRecognitionActor::SetLiveRecognition(bool live_recognition)
{
	this->live_recognition = live_recognition;
	return;
}

bool ASpeechRecognitionActor::GetLiveRecognition() {
	return live_recognition;
}

float ASpeechRecognitionActor::GetDelayTime() {
	return delay_time;
}

/**************************
// Change recognition methods
**************************/
bool ASpeechRecognitionActor::EnableKeywordMode(TArray<FRecognitionPhrase> wordList)
{
	if (listenerThread != NULL) {
		return listenerThread->EnableKeywordMode(wordList);
	}
	return false;
}

bool ASpeechRecognitionActor::EnableGrammarMode(FString grammarName)
{
	if (listenerThread != NULL) {
		return listenerThread->EnableGrammarMode(grammarName);
	}
	return false;
}

bool ASpeechRecognitionActor::EnablePhoneticMode()
{
	if (listenerThread != NULL) {
		return listenerThread->EnablePhoneticMode();
	}
	return false;
}

/**************************
// Callback methods
**************************/
void ASpeechRecognitionActor::WordsSpoken_trigger(FWordsSpokenSignature delegate_method, FRecognisedPhrases text)
{
	delegate_method.Broadcast(text);
}

void ASpeechRecognitionActor::WordsSpoken_method(FRecognisedPhrases text)
{
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
		FSimpleDelegateGraphTask::FDelegate::CreateStatic(&WordsSpoken_trigger, OnWordsSpoken, text)
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
