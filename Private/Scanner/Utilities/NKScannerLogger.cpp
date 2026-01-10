// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Utilities/NKScannerLogger.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/World.h"

// Initialize static singleton instance
UNKScannerLogger* UNKScannerLogger::GlobalInstance = nullptr;

UNKScannerLogger::UNKScannerLogger()
{
}

UNKScannerLogger::~UNKScannerLogger()
{
	UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Destructor called"));
}

void UNKScannerLogger::BeginDestroy()
{
	// Flush any remaining log data
	if (bLogFileInitialized && !ResolvedLogPath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Shutting down, final log path: %s"), *ResolvedLogPath);
	}
	
	// Remove from root to allow garbage collection
	if (this == GlobalInstance)
	{
		GlobalInstance = nullptr;
		RemoveFromRoot();
	}
	
	Super::BeginDestroy();
}

void UNKScannerLogger::Shutdown()
{
	if (GlobalInstance && IsValid(GlobalInstance))
	{
		GlobalInstance->RemoveFromRoot();
		GlobalInstance = nullptr;
		
		UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Global instance shutdown"));
	}
}

UNKScannerLogger* UNKScannerLogger::Get(UObject* WorldContextObject)
{
	// Return existing instance if valid
	if (GlobalInstance && IsValid(GlobalInstance))
	{
		return GlobalInstance;
	}
	
	// Create singleton instance if it doesn't exist
	if (WorldContextObject)
	{
		UWorld* World = WorldContextObject->GetWorld();
		if (!World)
		{
			return nullptr;
		}
		
		GlobalInstance = NewObject<UNKScannerLogger>(
			GetTransientPackage(),  // Use transient package instead of World
			UNKScannerLogger::StaticClass()
		);
		
		if (GlobalInstance)
		{
			// Prevent garbage collection
			GlobalInstance->AddToRoot();
			
			UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Global instance created"));
		}
	}
	
	return GlobalInstance;
}

void UNKScannerLogger::Log(const FString& Message, const FString& Category)
{
	LogInternal(Message, Category, ELogVerbosity::Log);
}

void UNKScannerLogger::LogWarning(const FString& Message, const FString& Category)
{
	LogInternal(Message, Category, ELogVerbosity::Warning);
}

void UNKScannerLogger::LogError(const FString& Message, const FString& Category)
{
	LogInternal(Message, Category, ELogVerbosity::Error);
}

void UNKScannerLogger::LogCustom(const FString& Message, const FString& Category, ELogVerbosity::Type Verbosity)
{
	LogInternal(Message, Category, Verbosity);
}

void UNKScannerLogger::LogInternal(const FString& Message, const FString& Category, ELogVerbosity::Type Verbosity)
{
	if (!bEnableLogging)
	{
		return;
	}
	
	// Safety check: don't log during shutdown
	if (!IsValid(this) || HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
	{
		return;
	}
	
	// Format the message
	FString FormattedMessage = FormatMessage(Message, Category);
	
	// Log to output window using UE_LOG
	switch (Verbosity)
	{
	case ELogVerbosity::Error:
		UE_LOG(LogTemp, Error, TEXT("%s"), *FormattedMessage);
		break;
	case ELogVerbosity::Warning:
		UE_LOG(LogTemp, Warning, TEXT("%s"), *FormattedMessage);
		break;
	default:
		UE_LOG(LogTemp, Log, TEXT("%s"), *FormattedMessage);
		break;
	}
	
	// Log to file if enabled
	if (bLogToFile)
	{
		WriteToLogFile(FormattedMessage);
	}
}

FString UNKScannerLogger::FormatMessage(const FString& Message, const FString& Category) const
{
	FString Result;
	
	// Add timestamp if enabled
	if (bIncludeTimestamp)
	{
		FDateTime CurrentTime = GetCurrentTime();
		FString Timestamp = CurrentTime.ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
		Result += FString::Printf(TEXT("[%s] "), *Timestamp);
	}
	
	// Add category if enabled
	if (bIncludeCategory && !Category.IsEmpty())
	{
		Result += FString::Printf(TEXT("[%s] "), *Category);
	}
	
	// Add the actual message
	Result += Message;
	
	return Result;
}

FString UNKScannerLogger::GetVerbosityString(ELogVerbosity::Type Verbosity) const
{
	switch (Verbosity)
	{
	case ELogVerbosity::Error:
		return TEXT("ERROR");
	case ELogVerbosity::Warning:
		return TEXT("WARN");
	case ELogVerbosity::Display:
		return TEXT("INFO");
	case ELogVerbosity::Verbose:
		return TEXT("VERBOSE");
	default:
		return TEXT("LOG");
	}
}

void UNKScannerLogger::WriteToLogFile(const FString& FormattedMessage)
{
	// Initialize log file path once
	if (!bLogFileInitialized)
	{
		// Use default filename if none specified
		FString PathToUse = LogFilePath.IsEmpty() ? GenerateDefaultLogFileName() : LogFilePath;
		
		// Resolve path (supports relative and absolute paths)
		if (FPaths::IsRelative(PathToUse))
		{
			ResolvedLogPath = FPaths::ProjectSavedDir() / TEXT("Logs") / PathToUse;
		}
		else
		{
			ResolvedLogPath = PathToUse;
		}
		
		// Ensure directory exists
		FString Directory = FPaths::GetPath(ResolvedLogPath);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		
		if (!PlatformFile.DirectoryExists(*Directory))
		{
			PlatformFile.CreateDirectoryTree(*Directory);
		}
		
		bLogFileInitialized = true;
		
		// Write header to new log file
		FDateTime HeaderTime = GetCurrentTime();
		FString TimezoneName = bUseEasternTime ? TEXT("Eastern Time") : TEXT("UTC");
		FString Header = FString::Printf(
			TEXT("========================================\n")
			TEXT("Scanner Log Started: %s (%s)\n")
			TEXT("Log File: %s\n")
			TEXT("========================================\n"),
			*HeaderTime.ToString(),
			*TimezoneName,
			*FPaths::GetCleanFilename(ResolvedLogPath)
		);
		
		FFileHelper::SaveStringToFile(
			Header,
			*ResolvedLogPath,
			FFileHelper::EEncodingOptions::AutoDetect,
			&IFileManager::Get(),
			FILEWRITE_Append
		);
		
		UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Logging to file: %s"), *ResolvedLogPath);
	}
	
	// Append message to file with newline
	FString MessageWithNewline = FormattedMessage + LINE_TERMINATOR;
	
	FFileHelper::SaveStringToFile(
		MessageWithNewline,
		*ResolvedLogPath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		FILEWRITE_Append
	);
}

void UNKScannerLogger::ClearLogFile()
{
	if (LogFilePath.IsEmpty())
	{
		return;
	}
	
	FString PathToClear = bLogFileInitialized ? ResolvedLogPath : GetResolvedLogFilePath();
	
	if (FPaths::FileExists(PathToClear))
	{
		IFileManager::Get().Delete(*PathToClear);
		bLogFileInitialized = false;
		
		UE_LOG(LogTemp, Log, TEXT("NKScannerLogger: Log file cleared: %s"), *PathToClear);
	}
}

FString UNKScannerLogger::GetResolvedLogFilePath() const
{
	if (bLogFileInitialized)
	{
		return ResolvedLogPath;
	}
	
	// Use default filename if none specified
	FString PathToResolve = LogFilePath.IsEmpty() ? GenerateDefaultLogFileName() : LogFilePath;
	
	if (FPaths::IsRelative(PathToResolve))
	{
		return FPaths::ProjectSavedDir() / TEXT("Logs") / PathToResolve;
	}
	
	return PathToResolve;
}

FDateTime UNKScannerLogger::GetCurrentTime() const
{
	FDateTime CurrentTime = FDateTime::Now();
	
	if (bUseEasternTime)
	{
		// Convert to Eastern Time (UTC-5 for EST, UTC-4 for EDT)
		// Note: This is a simple offset. For full DST support, you'd need to check dates
		FTimespan EasternOffset = FTimespan::FromHours(-5);  // EST offset
		
		// Simple DST check: between second Sunday in March and first Sunday in November
		int32 Year = CurrentTime.GetYear();
		int32 Month = CurrentTime.GetMonth();
		int32 Day = CurrentTime.GetDay();
		
		// Approximate DST period (March-November)
		bool bIsDST = (Month > 3 && Month < 11) || 
		              (Month == 3 && Day > 14) || 
		              (Month == 11 && Day < 7);
		
		if (bIsDST)
		{
			EasternOffset = FTimespan::FromHours(-4);  // EDT offset
		}
		
		CurrentTime += EasternOffset;
	}
	
	return CurrentTime;
}

FString UNKScannerLogger::GenerateDefaultLogFileName() const
{
	FDateTime CurrentTime = GetCurrentTime();
	FString TimezoneSuffix = bUseEasternTime ? TEXT("ET") : TEXT("UTC");
	
	// Format: ScannerLog_YYYYMMDD_HHMMSS_ET.log
	FString FileName = FString::Printf(
		TEXT("NKCameraScannerLog_%04d%02d%02d_%02d%02d%02d_%s.log"),
		CurrentTime.GetYear(),
		CurrentTime.GetMonth(),
		CurrentTime.GetDay(),
		CurrentTime.GetHour(),
		CurrentTime.GetMinute(),
		CurrentTime.GetSecond(),
		*TimezoneSuffix
	);
	
	return FileName;
}
