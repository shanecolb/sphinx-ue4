// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SpeechRecognition.h"

IMPLEMENT_MODULE( FSpeechRecognition, SpeechRecognition )

void FSpeechRecognition::StartupModule()
{
    if(PLATFORM_WINDOWS) {
        //Search project plugins folder for Dll
        FString dllName = "SphinxBase.dll";
        if (SearchForDllPath(FPaths::GamePluginsDir(), dllName))
        {
        }
        else if (SearchForDllPath(FPaths::EnginePluginsDir(), dllName)) //Failed in project dir, try engine plugins dir
        {
        }
        else
        {
            //Stop loading - plugin required DLL to load successfully
            checkf(false, TEXT("Failed to load dll"));
        }
        
        dllName = "PocketSphinx.dll";
        if (SearchForDllPath(FPaths::GamePluginsDir(), dllName))
        {
        }
        else if (SearchForDllPath(FPaths::EnginePluginsDir(), dllName)) //Failed in project dir, try engine plugins dir
        {
        }
        else
        {
            //Stop loading - plugin required DLL to load successfully
            checkf(false, TEXT("Failed to load dll"));
        }
    }
}

bool FSpeechRecognition::SearchForDllPath(FString _searchBase, FString _dllName)
{
	//Search Plugins folder for an instance of Dll.dll, and add to platform search path
	TArray<FString> directoriesToSkip;
	IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, directoriesToSkip, directoriesToSkip, false);
	PlatformFile.IterateDirectory(*_searchBase, Visitor);

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		const FString file = TimestampIt.Key();
		const FString filePath = FPaths::GetPath(file);
		const FString fileName = FPaths::GetCleanFilename(file);
		char* filename_str = TCHAR_TO_ANSI(*fileName);
		if (fileName.Equals(_dllName, ESearchCase::IgnoreCase))
		{
			FPlatformProcess::AddDllDirectory(*filePath); // only load dll when needed for use. Broken with 4.11.
			FPlatformProcess::GetDllHandle(*file); // auto-load dll with plugin - needed as 4.11 breaks above line.
			return true;
		}
	}
	return false;
}

void FSpeechRecognition::ShutdownModule()
{
	//TODO
}
