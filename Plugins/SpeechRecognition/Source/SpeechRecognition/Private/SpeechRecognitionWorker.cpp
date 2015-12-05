#include "SpeechRecognition.h"
#include "SpeechRecognitionWorker.h"

//General Log
DEFINE_LOG_CATEGORY(SpeechRecognitionPlugin);

FSpeechRecognitionWorker::FSpeechRecognitionWorker() {}

vector<string> FSpeechRecognitionWorker::Split(string s){
	regex r("\\w+");
	auto words_begin =
		std::sregex_iterator(s.begin(), s.end(), r);
	auto words_end = std::sregex_iterator();

	vector<string> result;

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		std::string match_str = match.str();
		result.push_back(match_str);
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

void FSpeechRecognitionWorker::SetLanguage(ESpeechRecognitionLanguage language) {

	// set Content Path
	FString contentPath = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir());
	contentPath_str = std::string(TCHAR_TO_UTF8(*contentPath));

	// language model and dictionary paths
	switch (language) {
	case ESpeechRecognitionLanguage::VE_English:
		langStr = "en-us";
		break;
	default:
		langStr = "en-us";
		break;
	}

}

void FSpeechRecognitionWorker::AddWords(TArray<FRecognitionKeyWord> keywords) {
	for (auto It = keywords.CreateConstIterator(); It; ++It)
	{
		FRecognitionKeyWord word = *It;
		std::string wordStr = std::string(TCHAR_TO_UTF8(*word.keyword));
		transform(wordStr.begin(), wordStr.end(), wordStr.begin(), ::tolower);

		ERecognitionKeywordTollerence tollerencEnum = word.tollerence;
		char* tollerence;
		switch (tollerencEnum) {
		case ERecognitionKeywordTollerence::VE_V_LOW:
			tollerence = "1e-40/";
			break;
		case ERecognitionKeywordTollerence::VE_LOW:
			tollerence = "1e-30/";
			break;
		case ERecognitionKeywordTollerence::VE_MEDIUM:
			tollerence = "1e-20/";
			break;
		case ERecognitionKeywordTollerence::VE_HIGH:
			tollerence = "1e-10/";
			break;
		case ERecognitionKeywordTollerence::VE_V_HIGH:
			tollerence = "1e-5/";
			break;
		default:
			tollerence = "1e-2/";
		}
		
		pair<map<string, char*>::iterator, bool> ret;
		ret = this->keywords.insert(pair<string, char*>(wordStr, tollerence));
		if (ret.second == false) {
			this->keywords.erase(wordStr);
			this->keywords.insert(pair<string, char*>(wordStr, tollerence));
		}
	}
}

bool FSpeechRecognitionWorker::Init() {

	std::string logPath = contentPath_str + "log/";
	std::string modelPath = contentPath_str + "model/" + langStr + "/" + langStr;
	std::string languageModel = contentPath_str + "model/" + langStr + "/" + langStr + ".lm.bin";
	std::string dictionaryPath = contentPath_str + "model/" + langStr + "/" + langStr + ".dict";

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

	// Start Sphinx
	config = cmd_ln_init(NULL, ps_args(), 1,
		"-hmm", modelPath.c_str(),
		"-dict", dictionaryPath.c_str(),
		"-bestpath", "yes",
		NULL);

	ps = ps_init(config);

	if (!Manager | !ps) {
		ClientMessage(FString(TEXT("Speech Recognition Thread failed to start")));
		initSuccess = false;
		return false;
	}

	// attempt to open the default recording device
	if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
		(int)cmd_ln_float32_r(config,
		"-samprate"))) == NULL) {
			ClientMessage(FString(TEXT("Failed to open audio device")));
			initSuccess = false;
			return initSuccess;
	}

	utt_started = 0;
	return true;
}

uint32 FSpeechRecognitionWorker::Run() {


	// attempt to open the default recording device
	if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
		(int)cmd_ln_float32_r(config,
		"-samprate"))) == NULL) {
		ClientMessage(FString(TEXT("Failed to open audio device")));
		return 1;
	}
	if (ad_start_rec(ad) < 0) {
		ClientMessage(FString(TEXT("Failed to start recording")));
		return 2;
	}

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
			if (dictionary.find(*v_It) == dictionary.end()){
				skip = true;
			}
		}

		if (skip)
			continue;

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
	
	if (ps_start_utt(ps) < 0) {
		ClientMessage(FString(TEXT("Failed to start utterance")));
		return 3;
	}

	while (StopTaskCounter.GetValue() == 0) {
		if ((k = ad_read(ad, adbuf, 1024)) < 0)
			ClientMessage(FString(TEXT("Failed to read audio")));
		ps_process_raw(ps, adbuf, k, 0, 0);
		in_speech = ps_get_in_speech(ps);
		if (in_speech && !utt_started) {
			utt_started = 1;
		}

		if (!in_speech && utt_started) {

			// obtain a count of the number of frames, and the hypothesis phrase spoken
			int32 frameCount = ps_get_n_frames(ps);
			int32 score;
			const char* hyp = ps_get_hyp(ps, &score);

			// re-loop, if there is no hypothesis
			if (hyp == NULL)
				continue;
			
			// log the phrase/phrases that have been spoken, and trigger WordSpoken method
			FString phrases = FString(ANSI_TO_TCHAR(hyp));
			ClientMessage(phrases);
			UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Phrases: %s "), *phrases);
			Manager->WordSpoken_method(phrases);

			// end utterence, and start listening for a new phrase
			ps_end_utt(ps);
			if (ps_start_utt(ps) < 0)
				ClientMessage(FString(TEXT("Failed to start")));
			utt_started = 0;
			UE_LOG(SpeechRecognitionPlugin, Log, TEXT("Listening..."));

		}
	}

	ad_close(ad);
	return 0;
}

void FSpeechRecognitionWorker::Stop() {
	StopTaskCounter.Increment();
}

bool FSpeechRecognitionWorker::StartThread(ASpeechRecognitionActor* manager) {
	Manager = manager;
	int32 threadIdx = ISpeechRecognition::Get().GetInstanceCounter();
	FString threadName = FString("FSpeechRecognitionWorker:") + FString::FromInt(threadIdx);
	initSuccess = true;
	Thread = FRunnableThread::Create(this, *threadName, 0U, TPri_Highest);
	return initSuccess;
}

void FSpeechRecognitionWorker::ClientMessage(FString text) {
	UE_LOG(SpeechRecognitionPlugin, Log, TEXT("%s"), *text);
}
