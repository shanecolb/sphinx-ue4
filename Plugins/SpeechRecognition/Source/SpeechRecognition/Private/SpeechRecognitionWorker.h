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
	VE_English 	UMETA(DisplayName = "English")
};

UENUM(BlueprintType)
enum class ERecognitionKeywordTollerence : uint8
{
	VE_V_LOW 	UMETA(DisplayName = "Very Low"),
	VE_LOW  	UMETA(DisplayName = "Low"),
	VE_MEDIUM	UMETA(DisplayName = "Medium"),
	VE_HIGH	    UMETA(DisplayName = "High"),
	VE_V_HIGH	UMETA(DisplayName = "Very High")
};

USTRUCT(BlueprintType)
struct FRecognitionKeyWord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString keyword;

	UPROPERTY(BlueprintReadWrite)
	ERecognitionKeywordTollerence tollerence;

	FRecognitionKeyWord(){

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

	void AddWords(TArray<FRecognitionKeyWord> dictionaryList);
	void SetLanguage(ESpeechRecognitionLanguage language);
	bool StartThread(ASpeechRecognitionActor* manager);

	static void ClientMessage(FString txt);

	void ShutDown();

};

