#pragma once

#include "Engine.h"
#include "SpeechRecognitionActor.h"
#include "SpeechRecognitionWorker.h"
#include "ISpeechRecognition.h"

class FSpeechRecognition :public ISpeechRecognition
{

public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Search for a dll to be loaded dynamically */
	bool SearchForDllPath(FString _searchBase, FString _dllName);

};
