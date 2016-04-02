#pragma once

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <pocketsphinx.h>
#include <stdio.h>
#include <time.h>
#include <regex>
#include <string>
#include <set>
#include <map>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <vector>
#include <utility>
#include "SpeechRecognitionWorker.generated.h"


UENUM(BlueprintType)
enum class ESpeechRecognitionLanguage : uint8
{
	VE_English 	UMETA(DisplayName = "English"),
	VE_Chinese  UMETA(DisplayName = "Chinese"),
	VE_French	UMETA(DisplayName = "French")
};

UENUM(BlueprintType)
enum class EPhraseRecognitionTolerance : uint8
{
	VE_1 	UMETA(DisplayName = "V1"),
	VE_2 	UMETA(DisplayName = "V2"),
	VE_3 	UMETA(DisplayName = "V3"),
	VE_4 	UMETA(DisplayName = "V4"),
	VE_5 	UMETA(DisplayName = "V5"),
	VE_6 	UMETA(DisplayName = "V6"),
	VE_7 	UMETA(DisplayName = "V7"),
	VE_8 	UMETA(DisplayName = "V8"),
	VE_9 	UMETA(DisplayName = "V9"),
	VE_10 	UMETA(DisplayName = "V10")
};

USTRUCT(BlueprintType)
struct FRecognitionPhrase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString keyword;

	UPROPERTY(BlueprintReadWrite)
	EPhraseRecognitionTolerance tolerance;

	FRecognitionPhrase(){

	}
};

//General Log
DECLARE_LOG_CATEGORY_EXTERN(SpeechRecognitionPlugin, Log, All);

#define SENSCR_SHIFT 10

class ASpeechRecognitionActor;

using namespace std;

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
	bool initSuccess;
	bool wordsAdded;

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

	//Stores the recognition keywords, along with their tollerences
	std::map <string , char*> keywords;

	//Dictionary
	std::set<std::string> dictionary;

	//Splits a string by whitespace
	std::vector<std::string> FSpeechRecognitionWorker::Split(std::string s);

public:
	FSpeechRecognitionWorker();
	virtual ~FSpeechRecognitionWorker();

	//FRunnable interface
	virtual bool Init();
	virtual void Stop();
	virtual uint32 Run();

	void AddWords(TArray<FRecognitionPhrase> dictionaryList);
	void SetLanguage(ESpeechRecognitionLanguage language);
	bool StartThread(ASpeechRecognitionActor* manager);

	static void ClientMessage(FString txt);

	void ShutDown();

};

