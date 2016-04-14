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

};
