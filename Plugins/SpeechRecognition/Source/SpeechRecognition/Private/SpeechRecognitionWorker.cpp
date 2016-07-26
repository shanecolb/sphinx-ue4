#include "SpeechRecognition.h"
#include "SpeechRecognitionWorker.h"

//General Log
DEFINE_LOG_CATEGORY(SpeechRecognitionPlugin);

FSpeechRecognitionWorker::FSpeechRecognitionWorker() {}

vector<string> FSpeechRecognitionWorker::Split(string s) {
	std::locale loc;
	regex r("\\s+");
	auto words_begin = std::sregex_iterator(s.begin(), s.end(), r);
	auto words_end = std::sregex_iterator();

	vector<string> result;

	// single word, no split necessary
	if (words_begin == words_end) {
		result.push_back(s);
	}
	else {
		stringstream ss(s); // Insert the string into a stream
		string buf;
		vector<string> tokens; // Create vector to hold our words
		while (ss >> buf)
			result.push_back(buf);
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


bool FSpeechRecognitionWorker::EnableGrammarMode(FString grammarName)
{
	const char* name = TCHAR_TO_ANSI(*grammarName);
	std::string grammarFile = contentPath_str + "model/" + langStr + "/grammars/" + name + ".gram";

	// Init config and set default params
	InitConfig();
	cmd_ln_set_str_r(config, "-jsgf", grammarFile.c_str());

	initRequired = true;
	detectionMode = ESpeechRecognitionMode::VE_GRAMMAR;
	return true;
}

bool FSpeechRecognitionWorker::EnableKeywordMode(TArray<FRecognitionPhrase> wordList)
{
	// Init config and set default params
	InitConfig();
	//cmd_ln_set_boolean_r(config, "-bestpath", true);

	AddWords(wordList);

	std::ifstream file(dictionaryPath);
	std::string currentLine;

	// load dictionary
	dictionary.clear();
	while (file.good())
	{
		std::getline(file, currentLine);
		std::string rawWord = currentLine.substr(0, currentLine.find(" "));
		std::string mappedWord = "";
		std::size_t beginBracket = rawWord.find("(");
		std::size_t endBracket = rawWord.find(")");
		if (beginBracket != std::string::npos && endBracket != std::string::npos)
		{
			mappedWord = rawWord.substr(0, beginBracket);
		}
		else {
			mappedWord = rawWord;
		}

		std::set<std::string> mappings;
		if (dictionary.find(mappedWord) != dictionary.end()) {
			mappings = dictionary.at(mappedWord);
			mappings.insert(rawWord);
			dictionary[mappedWord] = mappings;
		}
		mappings.insert(rawWord);
		dictionary.insert(make_pair(mappedWord, mappings));
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
	std::string languageString;

	// language model and dictionary paths
	switch (language) {
	case ESpeechRecognitionLanguage::VE_English:
		langStr = (char*)"en";
		break;
	case ESpeechRecognitionLanguage::VE_Chinese:
		langStr = (char*)"zn";
		break;
	case ESpeechRecognitionLanguage::VE_French:
		langStr = (char*)"fr";
		break;
	default:
		langStr = (char*)"en";
		break;
	}

}

void FSpeechRecognitionWorker::InitConfig() {
	argFilePath = contentPath_str + +"model/" + langStr + "/" + langStr + ".args";
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
		NULL);

	// loop over, and set sphinx params	
	for (FSpeechRecognitionParam param : sphinxParams)
	{
		if (param.type == ESpeechRecognitionParamType::VE_FLOAT)
		{
			cmd_ln_set_float_r(config, param.name, atof(param.value));
		}
		if (param.type == ESpeechRecognitionParamType::VE_BOOLEAN)
		{
			if (stricmp(param.value, "0") == 0)
			{
				cmd_ln_set_boolean_r(config, param.name, false);
			}
			if (stricmp(param.value, "1") == 0)
			{
				cmd_ln_set_boolean_r(config, param.name, true);
			}
		}
		if (param.type == ESpeechRecognitionParamType::VE_STRING)
		{
			cmd_ln_set_str_r(config, param.name, param.value);
		}
		if (param.type == ESpeechRecognitionParamType::VE_INTEGER)
		{
			cmd_ln_set_int_r(config, param.name, atoi(param.value));
		}
	}
	
	// reset params
	sphinxParams.Empty();

}

bool FSpeechRecognitionWorker::SetConfigParam(FString param, ESpeechRecognitionParamType type, FString value)
{
	char* paramName = TCHAR_TO_UTF8(*param);
	char* paramValue = TCHAR_TO_UTF8(*value);

	// Validate the incoming string, against the data type
	if (type == ESpeechRecognitionParamType::VE_FLOAT)
	{
		double validationFloat = atof(paramValue);
		if (validationFloat == 0.0F) {
			return false;
		}
		FSpeechRecognitionParam sphinxParam(paramName, type, paramValue);
		sphinxParams.Add(sphinxParam);
		return true;
	}

	if (type == ESpeechRecognitionParamType::VE_BOOLEAN)
	{
		if (value.Equals("true", ESearchCase::IgnoreCase)) {
			paramValue = "1";
			FSpeechRecognitionParam sphinxParam(paramName, type, paramValue);
			sphinxParams.Add(sphinxParam);
			return true;
		}
		if(value.Equals("false", ESearchCase::IgnoreCase)) {
			paramValue = "0";
			FSpeechRecognitionParam sphinxParam(paramName, type, paramValue);
			sphinxParams.Add(sphinxParam);
			return true;
		}
		return false;
	}

	if (type == ESpeechRecognitionParamType::VE_STRING)
	{
		FSpeechRecognitionParam sphinxParam(paramName, type, paramValue);
		sphinxParams.Add(sphinxParam);
		return true;
	}

	if (type == ESpeechRecognitionParamType::VE_INTEGER)
	{
		if (value.IsNumeric()) {
			FSpeechRecognitionParam sphinxParam(paramName, type, paramValue);
			sphinxParams.Add(sphinxParam);
			return true;
		}
		return false;
	}
	return false;
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


uint32 FSpeechRecognitionWorker::Run() {

	bool initComplete = false;
	uint8 prev_in_speech = 0;
	while (StopTaskCounter.GetValue() == 0) {

		// loop until we have initialised 
		if (initRequired) {

			ps = ps_init(config);

			if (!Manager | !ps) {
				ClientMessage(FString(TEXT("Speech Recognition Thread failed to start")));
				return 1;
			}

			ClientMessage(FString(TEXT("Speech Recognition has started")));

			if (detectionMode == ESpeechRecognitionMode::VE_KEYWORD) {

				// set key-phrases
				map<string, char*>::iterator it;
				char** phrases = (char**)malloc(keywords.size() * sizeof(char*));
				int32* tolerances;
				tolerances = (int32*)malloc(keywords.size() * sizeof(int32));
				int i = 0;

				for (it = keywords.begin(); it != keywords.end(); ++it) {

					// check if the word is in the dictionary. If missing, omit the phrase, and log
					vector<string> splitString = Split(it->first);
					vector<string>::iterator v_It;
					std::locale loc;

					bool skip = false;
					for (v_It = splitString.begin(); v_It != splitString.end(); ++v_It) {
						std::string orginalStr = "";
						for (char c : *v_It) if ((bool)std::isalpha(c, loc) || (int)c == 32) orginalStr += c;
						if (dictionary.find(orginalStr) == dictionary.end())
						{
							skip = true;
							continue;
						}
						if (dictionary.find(*v_It) == dictionary.end()) {
							std::set<string> wordSet = dictionary.at(orginalStr);
							if (wordSet.size() > 1) {
								std::string Strtxt = "The word '" + orginalStr + "' has multiple definitions, ensure multiple phrases (for each phonetic match) is added.";
								const char* txt = Strtxt.c_str();
								FString msg = FString(UTF8_TO_TCHAR(txt));
								UE_LOG(SpeechRecognitionPlugin, Log, TEXT("WARNING: %s "), *msg);
							}
							if (wordSet.find(*v_It) == wordSet.end()) {
								skip = true;
							}
						}
					}

					if (skip)
					{
						std::string Strtxt = "The phrase '" + it->first + "' can not be added, as it contains words that are not in the dictionary.";
						const char* txt = Strtxt.c_str();
						FString msg = FString(UTF8_TO_TCHAR(txt));
						UE_LOG(SpeechRecognitionPlugin, Log, TEXT("SKIPPED PHRASE: %s "), *msg);
						continue;
					}

					//add all variations of the keyword, to ensure dictionary variations are detected
					char* str;
					str = (char *)malloc(it->first.size() + 1);
					memcpy(str, it->first.c_str(), it->first.size() + 1);
					phrases[i] = str;

					//tolerance
					int32 toleranceInt = (int32)logmath_log(ps_get_logmath(ps), atof(it->second)) >> SENSCR_SHIFT;
					tolerances[i] = toleranceInt;
					i++;
				}

				int phraseCnt = i;
				const char** const_copy = (const char**)phrases;
				ps_set_keyphrase(ps, "keyphrase_search", const_copy, tolerances, phraseCnt);
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
		}
		else {
			if (initComplete == false) {
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

		if (!in_speech && utt_started) {

			// obtain a count of the number of frames, and the hypothesis phrase spoken
			int frame_rate = cmd_ln_int32_r(config, "-frate");
			int32 frameCount = ps_get_n_frames(ps);
			int32 score;

			// re-loop, if there is no hypothesis
			if (ps_get_hyp(ps, &score) == NULL)
				continue;

			TArray<FString> phraseSet;
			map<float, std::string> orderedPhrases;
			map<float, std::string>::iterator it;

			ps_seg_t *iter = ps_seg_iter(ps);

			// Orders the detected phrases into a map.
			// Key = seconds from the start of speech recognition detection.
			// Value = the phrase that was detected.
			while (iter != NULL) {
				int32 sf, ef;
				ps_seg_frames(iter, &sf, &ef);
				std::string word(ps_seg_word(iter));
				float startTime = ((float)sf / frame_rate);
				float endTime = ((float)ef / frame_rate);
				// For debug purposes. Logs the raw detected phrase + time they were detected
				UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Word: %s Start Time: %.3f End Time %.3f"), UTF8_TO_TCHAR(word.c_str()), startTime, endTime);
				orderedPhrases.insert(pair<float, std::string>(startTime, word));
				iter = ps_seg_next(iter);
			}

			// Loops through the ordered phrases (in detection order).
			// We only pickup and use the first detection, of each unique phrase. 
			// This is to handle words defined with multiple phonetic definitions
			for (it = orderedPhrases.begin(); it != orderedPhrases.end(); ++it) {
				std::locale loc;
				std::string originalHypothesis;
				std::string hypStr = it->second;
				// This ensures the additional definitions of the same word, are treated the same as one another				
				for (char c : hypStr) if ((bool)std::isalpha(c, loc) || (int)c == 32) originalHypothesis += c;
				FString phrase = FString(UTF8_TO_TCHAR(originalHypothesis.c_str()));
				if (detectionMode == ESpeechRecognitionMode::VE_KEYWORD)
				{
					if (!phraseSet.Contains(phrase))
					{
						phraseSet.Add(phrase);
						ClientMessage(phrase);
						UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Phrases: %s "), *phrase);
					}
				}
				if (detectionMode == ESpeechRecognitionMode::VE_GRAMMAR)
				{
					if(phrase != "sil")
					{
						phraseSet.Add(phrase);
						ClientMessage(phrase);
						UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Phrases: %s "), *phrase);
					}
				}

			}

			FRecognisedPhrases recognisedPhrases;
			recognisedPhrases.phrases = phraseSet;
			Manager->WordsSpoken_method(recognisedPhrases);


			// start listening for a new phrase
			ps_end_utt(ps);
			if (ps_start_utt(ps) < 0)
				ClientMessage(FString(TEXT("Failed to start")));
			utt_started = 0;
		}

	}

	ad_close(ad);
	ps_free(ps);
	cmd_ln_free_r(config);

	return 0;
}