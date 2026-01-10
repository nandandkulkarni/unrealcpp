// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NKScannerLogger.generated.h"

/**
 * Centralized logging utility for scanner system
 * Can log to output window and/or file
 */
UCLASS(BlueprintType)
class TPCPP_API UNKScannerLogger : public UObject
{
	GENERATED_BODY()

public:
	UNKScannerLogger();
	virtual ~UNKScannerLogger();
	virtual void BeginDestroy() override;
	
	// ===== Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bEnableLogging = true;  // Enabled by default
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bLogToFile = true;  // Enabled by default
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging", meta = (EditCondition = "bLogToFile"))
	FString LogFilePath;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bIncludeTimestamp = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bIncludeCategory = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Logging")
	bool bUseEasternTime = true;
	
	// ===== Logging Methods =====
	
	/** Log a general message */
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void Log(const FString& Message, const FString& Category = TEXT("Scanner"));
	
	/** Log a warning message */
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void LogWarning(const FString& Message, const FString& Category = TEXT("Scanner"));
	
	/** Log an error message */
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void LogError(const FString& Message, const FString& Category = TEXT("Scanner"));
	
	/** Log with custom verbosity */
	void LogCustom(const FString& Message, const FString& Category, ELogVerbosity::Type Verbosity);
	
	/** Clear the log file */
	UFUNCTION(BlueprintCallable, Category = "Logging")
	void ClearLogFile();
	
	/** Get the full resolved log file path */
	UFUNCTION(BlueprintPure, Category = "Logging")
	FString GetResolvedLogFilePath() const;
	
	// ===== Static Access (Singleton Pattern) =====
	
	/** Get or create the global scanner logger instance */
	static UNKScannerLogger* Get(UObject* WorldContextObject);
	
	/** Shutdown and cleanup the global logger instance */
	static void Shutdown();
	
private:
	// Internal logging
	void LogInternal(const FString& Message, const FString& Category, ELogVerbosity::Type Verbosity);
	void WriteToLogFile(const FString& FormattedMessage);
	FString FormatMessage(const FString& Message, const FString& Category) const;
	FString GetVerbosityString(ELogVerbosity::Type Verbosity) const;
	FDateTime GetCurrentTime() const;
	FString GenerateDefaultLogFileName() const;

	// File management
	bool bLogFileInitialized = false;
	FString ResolvedLogPath;
	
	// Singleton instance
	static UNKScannerLogger* GlobalInstance;
};
