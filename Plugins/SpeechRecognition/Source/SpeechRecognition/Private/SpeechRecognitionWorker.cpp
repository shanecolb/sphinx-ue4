#include "SpeechRecognition.h"
#include "SpeechRecognitionWorker.h"

//General Log
DEFINE_LOG_CATEGORY(SpeechRecognitionPlugin);

FSpeechRecognitionWorker::FSpeechRecognitionWorker() {}

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

void FSpeechRecognitionWorker::AddWords(TArray<FString> dictionaryList) {
	this->dictionaryList = dictionaryList;
}

bool FSpeechRecognitionWorker::Init() {

	std::string modelPath = contentPath_str + "model/" + langStr + "/" + langStr;
	std::string languageModel = contentPath_str + "model/" + langStr + "/" + langStr + ".lm.bin";
	std::string dictionaryPath = contentPath_str + "model/" + langStr + "/" + langStr + ".dict";

	// load dictionary
	dictionaryMap.clear();

	std::ifstream file(dictionaryPath);
	std::vector<std::string> words;
	std::string currentLine;

	while (file.good())
	{
		std::getline(file, currentLine);
		std::string word = currentLine.substr(0, currentLine.find(" "));
		std::string phrase = currentLine.substr(currentLine.find(" ") + 1, currentLine.size());
		dictionaryMap.insert(make_pair(word, phrase));
	}

	// Start Sphinx
	config = cmd_ln_init(NULL, ps_args(), 1,
		"-hmm", modelPath.c_str(),
		"-lm", languageModel.c_str(),
		NULL);
	
	ps = ps_init(config);

	if (!Manager | !ps) {
		ClientMessage(FString(TEXT("Speech Recognition Thread failed to start")));
		initSuccess = false;
		return false;
	}

	// only include the words/phrases that have been added
	for (auto It = dictionaryList.CreateConstIterator(); It; ++It)
	{
		FString word = *It;
		std::string wordStr = std::string(TCHAR_TO_UTF8(*word));
		if (dictionaryMap.find(wordStr) != dictionaryMap.end())
		{
			std::string phraseStr = dictionaryMap.at(wordStr);
			ps_add_word(ps, wordStr.c_str(), phraseStr.c_str(), TRUE);
		}
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

	char const *hyp;
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
			/* speech -> silence transition, time to start new utterance  */
			ps_end_utt(ps);
			hyp = ps_get_hyp(ps, NULL);
			if (hyp != NULL)
				Manager->WordSpoken_method(FString(hyp));

			if (ps_start_utt(ps) < 0)
				ClientMessage(FString(TEXT("Failed to start")));
			utt_started = 0;
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
