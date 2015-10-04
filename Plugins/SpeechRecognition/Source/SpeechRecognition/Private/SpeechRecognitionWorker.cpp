#include "SpeechRecognition.h"
#include "SpeechRecognitionWorker.h"

//General Log
DEFINE_LOG_CATEGORY(YourLog);

FSpeechRecognitionWorker::FSpeechRecognitionWorker() {}

FSpeechRecognitionWorker::~FSpeechRecognitionWorker() {
	delete Thread;
	Thread = NULL;
}

void FSpeechRecognitionWorker::ShutDown() {
	Stop();
	Thread->WaitForCompletion();
}

void FSpeechRecognitionWorker::SetLanguage(ESpeechRecognitionLanguage language){

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

void FSpeechRecognitionWorker::AddWords(TArray<FString> dictionaryList){
	for (auto It = dictionaryList.CreateConstIterator(); It; ++It)
	{
		FString word = *It;
		std::string wordStr = std::string(TCHAR_TO_UTF8(*word));

		// search for the word in the dictionary
		// if found, add the word to the sphinx process
		if (dictionaryMap.find(wordStr) != dictionaryMap.end())
		{
			std::string phraseStr = dictionaryMap.at(wordStr);
			ps_add_word(ps, wordStr.c_str(), phraseStr.c_str(), TRUE);
		}
	}
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
	
	ClientMessage(FString(modelPath.c_str()));
	ClientMessage(FString(languageModel.c_str()));

	// Start Sphinx
	config = cmd_ln_init(NULL, ps_args(), 1,
		"-hmm", modelPath.c_str(),
		"-lm", languageModel.c_str(),
		NULL);
	
	ps = ps_init(config);

	if (Manager && ps) {
		ClientMessage(FString(TEXT("Speech Recognition Thread started successfully")));
		return true;
	}
	else{
		ClientMessage(FString(TEXT("Speech Recognition Thread failed to start")));
		return false;
	}
}

uint32 FSpeechRecognitionWorker::Run() {

	// begin
	if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
		(int)cmd_ln_float32_r(config,
		"-samprate"))) == NULL)
		ClientMessage(FString(TEXT("Failed to open audio device")));
	if (ad_start_rec(ad) < 0)
		ClientMessage(FString(TEXT("Failed to start recording")));

	if (ps_start_utt(ps) < 0)
		ClientMessage(FString(TEXT("Failed to start utterance")));
	utt_started = 0;
	ClientMessage(FString(TEXT("Ready...")));

	char const *hyp;

	while (StopTaskCounter.GetValue() == 0) {
		if ((k = ad_read(ad, adbuf, 2048)) < 0)
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

void FSpeechRecognitionWorker::SetManager(ASpeechRecognitionActor* manager){
	Manager = manager;
	Thread = FRunnableThread::Create(this, TEXT("FSpeechRecognitionWorker"), 0U, TPri_BelowNormal);
}

void FSpeechRecognitionWorker::ClientMessage(FString text)
{
	UE_LOG(YourLog, Log, TEXT("%s"), *text);
}
