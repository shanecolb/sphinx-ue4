#pragma once

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <pocketsphinx.h>
#include <stdio.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <vector>

//using namespace std;

//General Log
DECLARE_LOG_CATEGORY_EXTERN(YourLog, Log, All);

UENUM(BlueprintType)
enum class ESpeechRecognitionLanguage : uint8
{
	VE_English 	UMETA(DisplayName = "English")
};

class ASpeechRecognitionActor;

class FSpeechRecognitionWorker :public FRunnable
{

private:
	// Sphinx
	ps_decoder_t *ps;
	cmd_ln_t *config;
	ad_rec_t *ad;
	int16 adbuf[2048];
	uint8 utt_started, in_speech;
	int32 k;

	//Thread
	FRunnableThread* Thread;

	//Pointer to our manager
	ASpeechRecognitionActor* Manager;

	//Thread safe counter 
	FThreadSafeCounter StopTaskCounter;

	//Language
	char* langStr = NULL;

	//Path to the content folder
	std::string contentPath_str;

	//Dictionary phrase map
	std::map<std::string, std::string> dictionaryMap;

public:
	FSpeechRecognitionWorker();
	virtual ~FSpeechRecognitionWorker();

	//FRunnable interface
	virtual bool Init();
	virtual void Stop();
	virtual uint32 Run();

	void AddWords(TArray<FString> dictionaryList);
	void SetLanguage(ESpeechRecognitionLanguage language);
	void SetManager(ASpeechRecognitionActor* manager);

	void ClientMessage(FString txt);

	void ShutDown();

};

