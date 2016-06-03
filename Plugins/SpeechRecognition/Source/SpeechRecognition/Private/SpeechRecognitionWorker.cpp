#include "SpeechRecognition.h"
#include "SpeechRecognitionActor.h"
#include "SpeechRecognitionWorker.h"

//General Log
DEFINE_LOG_CATEGORY(SpeechRecognitionPlugin);

FSpeechRecognitionWorker::FSpeechRecognitionWorker() {}

vector<string> FSpeechRecognitionWorker::Split(string s) {
	regex r("\\w+");
	auto words_begin = std::sregex_iterator(s.begin(), s.end(), r);
	auto words_end = std::sregex_iterator();

	vector<string> result;

	// single word, no split necessary
	if (words_begin == words_end) {
		result.push_back(s);
	}
	else {
		// multiple words, split it up
		for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
			std::smatch match = *i;
			std::string match_str = match.str();
			result.push_back(match_str);
		}
	}
	return result;
}

FSpeechRecognitionWorker::~FSpeechRecognitionWorker() {
	delete Thread;
	Thread = NULL;
}

void FSpeechRecognitionWorker::ShutDown() {
	Stop();
	Thread->WaitForCompletion();
	delete Thread;
}

bool FSpeechRecognitionWorker::EnablePhoneticMode()
{
	detectionMode = ESpeechRecognitionMode::VE_PHONETIC;
	return true;
}

bool FSpeechRecognitionWorker::EnableGrammarMode(FString grammarName)
{
	const char* name = TCHAR_TO_ANSI(*grammarName);
	std::string grammarFile = contentPath_str + "model/" + langStr + "/grammars/" + name + ".gram";
	
	// Init config and set default params
	InitConfig();
	cmd_ln_set_str_r(config, "-jsgf", grammarFile.c_str());
	cmd_ln_set_float_r(config, "-beam", 1e-20);
	cmd_ln_set_float_r(config, "-wbeam", 1e-30);
	cmd_ln_set_float_r(config, "-pbeam", 1e-20);

	initRequired = true;
	detectionMode = ESpeechRecognitionMode::VE_GRAMMAR;
	return true;
}

bool FSpeechRecognitionWorker::EnableKeywordMode(TArray<FRecognitionPhrase> wordList)
{
	// Init config and set default params
	InitConfig();
	cmd_ln_set_boolean_r(config, "-bestpath", true);

	AddWords(wordList);

	std::ifstream file(dictionaryPath);
	std::string currentLine;

	// load dictionary
	dictionary.clear();
	while (file.good())
	{
		std::getline(file, currentLine);
		std::string word = currentLine.substr(0, currentLine.find(" "));
		dictionary.insert(word);
	}

	initRequired = true;
	detectionMode = ESpeechRecognitionMode::VE_KEYWORD;
	return true;
}


void FSpeechRecognitionWorker::AddWords(TArray<FRecognitionPhrase> keywords) {
	this->keywords.clear();
	for (auto It = keywords.CreateConstIterator(); It; ++It)
	{
		FRecognitionPhrase word = *It;
		std::string wordStr = std::string(TCHAR_TO_UTF8(*word.phrase));
		transform(wordStr.begin(), wordStr.end(), wordStr.begin(), ::tolower);

		EPhraseRecognitionTolerance toleranceEnum = word.tolerance;
		char* tolerance;
		switch (toleranceEnum) {
		case EPhraseRecognitionTolerance::VE_1:
			tolerance = (char*)"1e-60/";
			break;
		case EPhraseRecognitionTolerance::VE_2:
			tolerance = (char*)"1e-55/";
			break;
		case EPhraseRecognitionTolerance::VE_3:
			tolerance = (char*)"1e-50/";
			break;
		case EPhraseRecognitionTolerance::VE_4:
			tolerance = (char*)"1e-40/";
			break;
		case EPhraseRecognitionTolerance::VE_5:
			tolerance = (char*)"1e-30/";
			break;
		case EPhraseRecognitionTolerance::VE_6:
			tolerance = (char*)"1e-20/";
			break;
		case EPhraseRecognitionTolerance::VE_7:
			tolerance = (char*)"1e-10/";
			break;
		case EPhraseRecognitionTolerance::VE_8:
			tolerance = (char*)"1e-5/";
			break;
		case EPhraseRecognitionTolerance::VE_9:
			tolerance = (char*)"1e-4/";
			break;
		case EPhraseRecognitionTolerance::VE_10:
			tolerance = (char*)"1e-3/";
			break;
		default:
			tolerance = (char*)"1e-2/";
		}

		pair<map<string, char*>::iterator, bool> ret;
		ret = this->keywords.insert(pair<string, char*>(wordStr, tolerance));
		if (ret.second == false) {
			this->keywords.erase(wordStr);
			this->keywords.insert(pair<string, char*>(wordStr, tolerance));
		}
	}
}

void FSpeechRecognitionWorker::SetLanguage(ESpeechRecognitionLanguage language) {

	// set Content Path
	FString contentPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir());
	contentPath_str = std::string(TCHAR_TO_UTF8(*contentPath));

	// language model and dictionary paths
	switch (language) {
	case ESpeechRecognitionLanguage::VE_English:
		langStr = "en";
		break;
	case ESpeechRecognitionLanguage::VE_Chinese:
		langStr = "zn";
		break;
	case ESpeechRecognitionLanguage::VE_French:
		langStr = "fr";
		break;
	default:
		langStr = "en";
		break;
	}

}


void FSpeechRecognitionWorker::InitConfig() {
	logPath = contentPath_str + "log/";
	modelPath = contentPath_str + "model/" + langStr + "/" + langStr;
	languageModel = contentPath_str + "model/" + langStr + "/" + langStr + ".lm";
	dictionaryPath = contentPath_str + "model/" + langStr + "/" + langStr + ".dict";
	
	if (ps != NULL) {
		cmd_ln_free_r(config);
	}

	config = cmd_ln_init(NULL, ps_args(), 1,
		"-hmm", modelPath.c_str(),
		"-dict", dictionaryPath.c_str(),
		"-agc", "max",
		NULL);
}

uint32 FSpeechRecognitionWorker::Run() {

	bool initComplete = false;
	uint8 prev_in_speech = 0;

	float elapsed_time_ms;
	clock_t prev_time = clock();

	while (StopTaskCounter.GetValue() == 0) {
		// loop until we have initialised 
		if (initRequired) {

			/*
			if (ps != NULL) {
				ps_free(ps);				
			}
			*/

			ps_default_search_args(config);
			ps = ps_init(config);
			
			if (!Manager | !ps) {
				ClientMessage(FString(TEXT("Speech Recognition Thread failed to start")));
				return 1;
			}

			if (detectionMode == ESpeechRecognitionMode::VE_KEYWORD) {

				// set key-phrases
				map<string, char*>::iterator it;
				char** phrases = (char**)malloc(keywords.size() * sizeof(char*));
				int32* tollerences;
				tollerences = (int32*)malloc(keywords.size() * sizeof(int32));
				int i = 0;

				for (it = keywords.begin(); it != keywords.end(); ++it) {

					// check if the word is in the dictionary. If missing, omit the phrase, and log
					vector<string> splitString = Split(it->first);
					vector<string>::iterator v_It;

					bool skip = false;
					for (v_It = splitString.begin(); v_It != splitString.end(); ++v_It) {
						if (dictionary.find(*v_It) == dictionary.end()) {
							skip = true;
						}
					}

					if (skip)
					{
						std::string Strtxt = "The phrase '" + it->first + "' can not be added, as it contains words that are not in the dictionary.";
						const char* txt = Strtxt.c_str();
						// "' contains words that do not exist in the dictionary. Ignoring phrase.";
						FString msg = FString(UTF8_TO_TCHAR(txt));
						UE_LOG(SpeechRecognitionPlugin, Log, TEXT("SKIPPED PHRASE: %s "), *msg);
						continue;
					}

					// keyword
					char* str;
					str = (char *)malloc(it->first.size() + 1);
					memcpy(str, it->first.c_str(), it->first.size() + 1);
					phrases[i] = str;

					//tollerence
					int32 tollerenceInt = (int32)logmath_log(ps_get_logmath(ps), atof(it->second)) >> SENSCR_SHIFT;
					tollerences[i] = tollerenceInt;
					i++;
				}

				int phraseCnt = i;
				const char** const_copy = (const char**)phrases;
				ps_set_keyphrase(ps, "keyphrase_search", const_copy, tollerences, phraseCnt);
				ps_set_search(ps, "keyphrase_search");
			}

			// attempt to open the default recording device
			if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
				(int)cmd_ln_float32_r(config,
					"-samprate"))) == NULL) {
				ClientMessage(FString(TEXT("Failed to open audio device")));
				return 2;
			}

			if (ad_start_rec(ad) < 0) {
				ClientMessage(FString(TEXT("Failed to start recording")));
				return 3;
			}

			if (ps_start_utt(ps) < 0) {
				ClientMessage(FString(TEXT("Failed to start utterance")));
				return 4;
			}

			initRequired = false;
			initComplete = true;
			utt_started = 0;
		}else {
			if(initComplete == false) {
				FPlatformProcess::Sleep(0.01f);
				if (StopTaskCounter.GetValue() == 1) {
					return 0;
				}
				continue;
			}
		}
		
		if ((k = ad_read(ad, adbuf, 1024)) < 0)
			ClientMessage(FString(TEXT("Failed to read audio")));

		ps_process_raw(ps, adbuf, k, 0, 0);
		prev_in_speech = in_speech;
		in_speech = ps_get_in_speech(ps);

		// clear detection for when speech starts and stops
		if (in_speech != prev_in_speech) {
			if (in_speech) {
				Manager->StartedSpeaking_method();
			}
			else {
				Manager->StoppedSpeaking_method();
			}
		}

		if (in_speech && !utt_started) {
			utt_started = 1;
			ClientMessage(FString(TEXT("Listening")));
		}

		// change how we process incoming sound based on if live recognition is enabled or not
		if (Manager->GetLiveRecognition()) {
			// make sure we don't check too often for complete phrases
			elapsed_time_ms = (clock() - prev_time) / (CLOCKS_PER_SEC / 1000);
			if (elapsed_time_ms >= Manager->GetDelayTime() * 1000) {
				prev_time = clock();
				// get the hyp for anything recorded so far
				const char* hyp = ps_get_hyp(ps, NULL);

				// if speech so far contains any matching phrases, process them and start over
				if (hyp != NULL) {
					ps_end_utt(ps);

					ProcessIncomingSound();

					if (ps_start_utt(ps) < 0)
						ClientMessage(FString(TEXT("Failed to start")));
					utt_started = 0;
				}
			}
		}
		else {
			// when speaking has stopped
			if (!in_speech && utt_started) {
				ps_end_utt(ps);
				// get the hyp for all recorded speech
				const char* hyp = ps_get_hyp(ps, NULL);

				// if recorded speech contains matching phrases, process them
				if (hyp != NULL) {
					ProcessIncomingSound();
				}

				// start listening for a new phrase
				if (ps_start_utt(ps) < 0)
					ClientMessage(FString(TEXT("Failed to start")));
				utt_started = 0;
			}		
		}
	}
	ad_close(ad);
	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}

void FSpeechRecognitionWorker::ProcessIncomingSound() {
	int frame_rate = cmd_ln_int32_r(config, "-frate");
	ps_seg_t *iter = ps_seg_iter(ps);

	map<float, FString> orderedPhrases;
	map<float, FString>::iterator it;

	while (iter != NULL) {
		int32 sf, ef;
		ps_seg_frames(iter, &sf, &ef);
		FString word = FString(ps_seg_word(iter));
		float startTime = ((float)sf / frame_rate);
		float endTime = ((float)ef / frame_rate);
		UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Word: %s Start Time: %.3f End Time %.3f"), *word, startTime,
			endTime);
		iter = ps_seg_next(iter);
		orderedPhrases.insert(pair<float, FString>(startTime, word));
	}

	TArray<FString> phrases;
	for (it = orderedPhrases.begin(); it != orderedPhrases.end(); ++it) {
		FString word = it->second;
		if (!word.Equals(TEXT("<sil>"), ESearchCase::IgnoreCase))
		{
			UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Phrase: %s "), *word);
			phrases.Add(word);
		}
	}

	FRecognisedPhrases recogPhrases;
	recogPhrases.phrases = phrases;
	Manager->WordsSpoken_method(recogPhrases);

	return;
}

void FSpeechRecognitionWorker::Stop() {
	StopTaskCounter.Increment();
}

bool FSpeechRecognitionWorker::StartThread(ASpeechRecognitionActor* manager) {
	Manager = manager;
	int32 threadIdx = ISpeechRecognition::Get().GetInstanceCounter();
	FString threadName = FString("FSpeechRecognitionWorker:") + FString::FromInt(threadIdx);
	Thread = FRunnableThread::Create(this, *threadName, 0U, TPri_Highest);
	return true;
}

void FSpeechRecognitionWorker::ClientMessage(FString text) {
	UE_LOG(SpeechRecognitionPlugin, Log, TEXT("%s"), *text);
}
