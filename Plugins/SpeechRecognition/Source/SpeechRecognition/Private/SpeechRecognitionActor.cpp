#include "SpeechRecognition.h"
#include "SpeechRecognitionActor.h"

#define SPEECHRECOGNITIONPLUGIN ISpeechRecognition::Get()

bool ASpeechRecognitionActor::Init(ESpeechRecognitionLanguage language)
{
	if (listenerThread == NULL)
	{
		// start listener thread
		listenerThread = new FSpeechRecognitionWorker();
		TArray<FString> dictionaryList;
		listenerThread->SetLanguage(language);
		listenerThread->SetManager(this);
	}
	return (listenerThread == NULL);;
}

void ASpeechRecognitionActor::Shutdown()
{
	listenerThread->ShutDown();
	listenerThread = NULL;
}

void ASpeechRecognitionActor::AddWords(TArray<FString> wordList)
{
	listenerThread->AddWords(wordList);
}

void ASpeechRecognitionActor::WordSpoken_method(FString text)
{
	this->OnWordSpoken.Broadcast(text);
}