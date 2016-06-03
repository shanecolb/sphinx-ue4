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
#include <ctime>
#include <vector>
#include <utility>


//General Log
DECLARE_LOG_CATEGORY_EXTERN(SpeechRecognitionPlugin, Log, All);

#define SENSCR_SHIFT 10

class ASpeechRecognitionActor;

using namespace std;

class FSpeechRecognitionWorker :public FRunnable
{

private:
	// Sphinx
	ps_decoder_t *ps = NULL;
	cmd_ln_t *config = NULL;
	ad_rec_t *ad;
	int16 adbuf[2048];
	uint8 utt_started, in_speech;
	int32 k;
	bool initRequired = false;
	bool wordsAdded = false;

	//Speech detection mode
	ESpeechRecognitionMode detectionMode;

	//Thread
	FRunnableThread* Thread;

	//Pointer to our manager
	ASpeechRecognitionActor* Manager;

	//Thread safe counter 
	FThreadSafeCounter StopTaskCounter;

	//Language
	char* langStr = NULL;

	//Paths
	std::string contentPath_str;
	std::string logPath;
	std::string modelPath;
	std::string languageModel;
	std::string dictionaryPath;


	//Stores the recognition keywords, along with their tollerences
	std::map <string , char*> keywords;

	//Dictionary
	std::set<std::string> dictionary;

	//Splits a string by whitespace
	std::vector<std::string> FSpeechRecognitionWorker::Split(std::string s);

	//Create default Pocketsphinx constructor
	void InitConfig();

	//Add phrases to keyword match list (only required for Keyword matching)
	void AddWords(TArray<FRecognitionPhrase> dictionaryList);

	void ProcessIncomingSound();

public:
	FSpeechRecognitionWorker();
	virtual ~FSpeechRecognitionWorker();

	//FRunnable interface
	virtual void Stop();
	virtual uint32 Run();

	//Methods to switch recognition modes
	bool EnableKeywordMode(TArray<FRecognitionPhrase> wordList);
	bool EnableGrammarMode(FString grammarName);
	bool EnablePhoneticMode();

	void SetLanguage(ESpeechRecognitionLanguage language);
	bool StartThread(ASpeechRecognitionActor* manager);
	void ShutDown();

	//Print debug text
	static void ClientMessage(FString txt);

};

