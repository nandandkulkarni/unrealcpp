// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "NKScannerCameraActor.generated.h"

// Scanner state for terrain mapping workflow
UENUM(BlueprintType)
enum class EScannerState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Validating UMETA(DisplayName = "Validating Target"),
	Mapping UMETA(DisplayName = "Terrain Mapping"),
	Complete UMETA(DisplayName = "Complete")
};

// Data structure to store individual scan point information
USTRUCT(BlueprintType)
struct FScanDataPoint
{
	GENERATED_BODY()

	// Camera transform when this scan was taken
	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	FVector CameraPosition = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	FRotator CameraRotation = FRotator::ZeroRotator;

	// Laser hit information
	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	FVector LaserHitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	FVector LaserHitNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	float HitDistance = 0.0f;

	// Frame and timing information
	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	int32 FrameNumber = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	float TimeStamp = 0.0f;

	// Additional metadata
	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	FString HitActorName;

	UPROPERTY(BlueprintReadWrite, Category = "Scan Data")
	float OrbitAngle = 0.0f;
};

// Delegate declaration for laser hit events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnLaserHit,
	FVector, HitLocation,
	AActor*, HitActor,
	float, Distance
);

// Delegate for scan completion
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnScanComplete,
	FString, JSONFilePath
);

/**
 * NKScanner Camera Actor - A specialized cinematic camera for scanning
 * Inherits from ACineCameraActor to get all cinematic camera features
 * 
 * Features:
 * - Autonomous camera navigation around landscape/objects
 * - Laser scanning with robust surface detection
 * - JSON recording and playback for cinematic sequences
 * - Extensive logging to both UE_LOG and custom log files
 */
UCLASS(Blueprintable, ClassGroup = (AAScanner))
class TPCPP_API ANKScannerCameraActor : public ACineCameraActor
{
	GENERATED_BODY()

public:
	// Constructor
	ANKScannerCameraActor(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// ========================================================================
	// LOGGING PROPERTIES
	// ========================================================================
	
	/** Enable extensive logging to UE log and custom log file */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Logging")
	bool bEnableVerboseLogging;

	/** Custom log file path (leave empty for default: Saved/Logs/NKScanner.log) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Logging")
	FString CustomLogFilePath;

	/** Enable logging to custom file */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Logging")
	bool bLogToFile;

	/** Log every frame during scanning (can be very verbose!) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Logging")
	bool bLogEveryFrame;

	// ========================================================================
	// BASIC SCANNER PROPERTIES
	// ========================================================================
	
	/** Custom scanner-specific property - example: scan range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner")
	float ScanRange;

	/** Custom scanner-specific property - example: scan speed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner")
	float ScanSpeed;

	/** Enable/disable scanner functionality */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner")
	bool bScannerEnabled;

	// ========================================================================
	// LASER PROPERTIES
	// ========================================================================
	
	/** Maximum laser range in centimeters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	float LaserMaxRange;

	/** Enable laser visualization */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	bool bShowLaserBeam;

	/** Color of the laser beam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	FColor LaserColor;

	/** Thickness of the laser beam */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	float LaserThickness;

	/** Enable continuous laser shooting every frame */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	bool bContinuousLaserShoot;

	/** Channels to trace against (Default, WorldStatic, WorldDynamic, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Laser")
	TEnumAsByte<ECollisionChannel> LaserTraceChannel;

	// ========================================================================
	// AUDIO PROPERTIES
	// ========================================================================
	
	/** Enable audio feedback during scanning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio")
	bool bEnableAudioFeedback;
	
	/** Sound to play during validation/discovery phase (rapid beeping) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio")
	USoundBase* ValidationSound;
	
	/** Sound to play during terrain mapping phase (slower beeping) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio")
	USoundBase* MappingSound;
	
	/** Sound to play when target is found (confirmation beep) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio")
	USoundBase* TargetFoundSound;
	
	/** Sound to play when validation fails (error tone) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio")
	USoundBase* ValidationFailedSound;
	
	/** Time between validation sound plays in seconds (faster = more frequent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float ValidationSoundInterval;
	
	/** Time between mapping sound plays in seconds (slower = less frequent) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MappingSoundInterval;
	
	/** Volume multiplier for all scanner sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AudioVolumeMultiplier;
	
	// ========================================================================
	// TEXT-TO-SPEECH PROPERTIES
	// ========================================================================
	
	/** Enable Text-to-Speech announcements for important events */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|TextToSpeech")
	bool bEnableTextToSpeech;
	
	/** Volume for TTS announcements (0.0 - 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|TextToSpeech", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TTSVolume;
	
	/** Speech rate for TTS (0.5 = slow, 1.0 = normal, 2.0 = fast) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|TextToSpeech", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float TTSRate;

	// ========================================================================
	// CINEMATIC SCAN PROPERTIES
	// ========================================================================
	
	/** Target landscape or object to scan */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic")
	AActor* CinematicTargetLandscape;

	/** Height as percentage from bottom of target object (0-100) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic", meta = (ClampMin = "0", ClampMax = "100"))
	float CinematicHeightPercent;

	/** Distance to maintain from surface in meters */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic", meta = (ClampMin = "1"))
	float CinematicDistanceMeters;

	/** How many degrees to rotate per movement step */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic", meta = (ClampMin = "0.1", ClampMax = "10"))
	float CinematicAngularStepDegrees;
	
	/** How many degrees to rotate per validation step (Step 3 - larger = faster but less accurate) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic|Performance", meta = (ClampMin = "1", ClampMax = "45"))
	float ValidationAngularStepDegrees;
	
	/** How often to update terrain mapping in seconds (0.1 = 10 updates/sec, lower = faster but less smooth) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic|Performance", meta = (ClampMin = "0.016", ClampMax = "1.0"))
	float MappingUpdateInterval;

	/** File path for JSON output */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Cinematic")
	FString CinematicJSONOutputPath;

	// ========================================================================
	// PLAYBACK PROPERTIES
	// ========================================================================
	
	/** Playback speed multiplier (1.0 = normal, 2.0 = 2x speed, 0.5 = half speed) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Playback", meta = (ClampMin = "0.1", ClampMax = "10"))
	float PlaybackSpeedMultiplier;

	/** Enable looping playback */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|Playback")
	bool bLoopPlayback;

	// ========================================================================
	// HUD SETTINGS
	// ========================================================================
	
	/** Show debug HUD overlay with scanner status */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AAScanner|HUD")
	bool bShowDebugHUD;

	// ========================================================================
	// PUBLIC GETTER FUNCTIONS (For HUD Access)
	// ========================================================================
	
	// Scanner state getters
	EScannerState GetScannerState() const { return ScannerState; }
	bool IsScannerEnabled() const { return bScannerEnabled; }
	
	// Validation getters
	bool IsValidating() const { return bIsValidating; }
	int32 GetValidationAttempts() const { return ValidationAttempts; }
	float GetCurrentValidationAngle() const { return CurrentValidationAngle; }
	
	// Laser getters
	bool GetLastShotHit() const { return bLastShotHit; }
	float GetLaserMaxRange() const { return LaserMaxRange; }
	
	// Audio getters
	bool IsAudioEnabled() const { return bEnableAudioFeedback; }
	
	// Scan data getters
	bool IsCinematicScanActive() const { return bIsCinematicScanActive; }
	float GetCurrentOrbitAngle() const { return CurrentOrbitAngle; }
	int32 GetRecordedDataCount() const { return RecordedScanData.Num(); }
	float GetCinematicScanElapsedTime() const { return CinematicScanElapsedTime; }

	// ========================================================================
	// EVENTS
	// ========================================================================
	
	/** Event fired when laser hits something - Blueprint assignable */
	UPROPERTY(BlueprintAssignable, Category = "AAScanner|Events")
	FOnLaserHit OnLaserHit;

	/** Event fired when cinematic scan completes */
	UPROPERTY(BlueprintAssignable, Category = "AAScanner|Events")
	FOnScanComplete OnScanComplete;

	// ========================================================================
	// LASER RESULTS (Read-Only)
	// ========================================================================
	
	/** The last hit location from the laser (read-only, exposed to Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	FVector LastHitLocation;

	/** The last hit normal from the laser (read-only, exposed to Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	FVector LastHitNormal;

	/** The last actor hit by the laser (read-only, exposed to Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	AActor* LastHitActor;

	/** The last component hit by the laser (read-only, exposed to Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	UPrimitiveComponent* LastHitComponent;

	/** Distance to the last hit point */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	float LastHitDistance;

	/** Whether the last laser shot hit something */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	bool bLastShotHit;

	/** Name of the surface material hit (if any) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AAScanner|Laser|Results")
	FName LastHitPhysicalMaterial;

	// ========================================================================
	// BASIC SCANNER FUNCTIONS
	// ========================================================================
	
	/** Blueprint-callable function to start scanning */
	UFUNCTION(BlueprintCallable, Category = "AAScanner")
	void StartScanning();

	/** Blueprint-callable function to stop scanning */
	UFUNCTION(BlueprintCallable, Category = "AAScanner")
	void StopScanning();

	/** Blueprint-callable function to get scan progress */
	UFUNCTION(BlueprintPure, Category = "AAScanner")
	float GetScanProgress() const;

	/** Blueprint-callable function to shoot a laser from the camera */
	UFUNCTION(BlueprintCallable, Category = "AAScanner|Laser")
	bool ShootLaser();

	/** Blueprint-callable function to get the last hit location */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Laser")
	FVector GetLastHitLocation() const { return LastHitLocation; }

	/** Blueprint-callable function to get the last hit actor */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Laser")
	AActor* GetLastHitActor() const { return LastHitActor; }

	/** Blueprint-callable function to get the distance to last hit */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Laser")
	float GetLastHitDistance() const { return LastHitDistance; }

	/** Blueprint-callable function to check if last shot hit something */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Laser")
	bool DidLastShotHit() const { return bLastShotHit; }

	// ========================================================================
	// CINEMATIC SCANNING FUNCTIONS
	// ========================================================================
	
	/**
	 * Start autonomous cinematic scanning of a target landscape/object
	 * Camera will orbit around target at specified height and distance
	 * 
	 * @param TargetLandscape - The actor to scan around
	 * @param HeightPercent - Height as percentage of target bounds (0-100)
	 * @param DistanceMeters - Distance to maintain from surface in meters
	 * @param OutputJSONPath - File path where scan data will be saved
	 */
	UFUNCTION(BlueprintCallable, Category = "AAScanner|Cinematic", meta = (DisplayName = "Start Cinematic Scan"))
	void StartCinematicScan(
		AActor* TargetLandscape,
		float HeightPercent = 50.0f,
		float DistanceMeters = 50.0f,
		FString OutputJSONPath = ""
	);

	/**
	 * Stop the current cinematic scan and save JSON data
	 */
	UFUNCTION(BlueprintCallable, Category = "AAScanner|Cinematic", meta = (DisplayName = "Stop Cinematic Scan"))
	void StopCinematicScan();

	// ========================================================================
	// PLAYBACK FUNCTIONS
	// ========================================================================
	
	/**
	 * Start playback from previously recorded JSON scan data
	 * Call this from Sequencer to replay a recorded camera path
	 * 
	 * @param JSONFilePath - Path to the JSON file containing scan data
	 * @param PlaybackSpeed - Speed multiplier (1.0 = normal speed)
	 * @param bLoop - Should the playback loop when it reaches the end
	 */
	UFUNCTION(BlueprintCallable, Category = "AAScanner|Playback", meta = (DisplayName = "Start JSON Playback"))
	void StartJSONPlayback(
		FString JSONFilePath,
		float PlaybackSpeed = 1.0f,
		bool bLoop = false
	);

	/**
	 * Stop current playback
	 */
	UFUNCTION(BlueprintCallable, Category = "AAScanner|Playback", meta = (DisplayName = "Stop Playback"))
	void StopPlayback();

	/**
	 * Get current playback progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Playback")
	float GetPlaybackProgress() const;

	// ========================================================================
	// DATA ACCESS FUNCTIONS
	// ========================================================================
	
	/**
	 * Get all recorded scan data points
	 */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Data")
	TArray<FScanDataPoint> GetRecordedScanData() const { return RecordedScanData; }

	/**
	 * Get number of recorded scan points
	 */
	UFUNCTION(BlueprintPure, Category = "AAScanner|Data")
	int32 GetScanDataCount() const { return RecordedScanData.Num(); }

	// ========================================================================
	// LOGGING FUNCTIONS
	// ========================================================================
	
	/**
	 * Custom logger function that writes to both UE_LOG and custom file
	 * @param Message - The message to log
	 * @param bForceLog - Force logging even if verbose logging is disabled
	 */
	void LogMessage(const FString& Message, bool bForceLog = false);

	/**
	 * Initialize the custom log file
	 */
	void InitializeLogFile();

	/**
	 * Close and flush the custom log file
	 */
	void CloseLogFile();

private:
	// ========================================================================
	// INTERNAL STATE
	// ========================================================================
	
	// Basic scanning state
	float ScanProgress;
	bool bIsScanning;

	// Cinematic scan state
	bool bIsCinematicScanActive;
	float CurrentOrbitAngle;
	FVector CinematicLookAtTarget;
	FVector CinematicOrbitCenter;
	float CinematicOrbitRadius;
	float CinematicOrbitHeight;
	int32 CurrentScanFrameNumber;
	float CinematicScanElapsedTime;
	float CinematicScanUpdateAccumulator;  // Accumulator for controlling update frequency
	
	// Scanner state machine
	EScannerState ScannerState;
	
	// Validation state
	bool bIsValidating;
	float CurrentValidationAngle;
	int32 ValidationAttempts;
	float FirstHitAngle;
	FHitResult FirstHitResult;

	// Playback state
	bool bIsPlayingBack;
	int32 CurrentPlaybackFrame;
	float PlaybackFrameAccumulator;
	
	// Audio state
	UPROPERTY()
	UAudioComponent* ScannerAudioComponent;
	
	float AudioTimeSinceLastPlay;
	EScannerState LastAudioState;

	// Data storage
	UPROPERTY()
	TArray<FScanDataPoint> RecordedScanData;

	// Logging state
	FString ActualLogFilePath;
	bool bLogFileInitialized;

	// ========================================================================
	// INTERNAL HELPER FUNCTIONS
	// ========================================================================
	
	// Laser tracing
	bool PerformLaserTrace(FHitResult& OutHitResult);
	void UpdateLaserHitProperties(const FHitResult& HitResult, bool bHit);
	void DrawLaserBeam(const FVector& Start, const FVector& End, bool bHit);

	// Cinematic scanning
	void UpdateCinematicScan(float DeltaTime);
	void InitializeCinematicScan();
	void RecordCurrentScanPoint();
	FVector CalculateOrbitPosition(float Angle);
	FRotator CalculateLookAtRotation(const FVector& CameraPosition);
	
	// Target Finder state machine
	void UpdateTargetFinder(float DeltaTime);
	void StartTargetFinderState();
	void OnTargetFinderSuccess();
	void OnTargetFinderFailure();

	// JSON operations
	bool SaveScanDataToJSON(const FString& FilePath);
	bool LoadScanDataFromJSON(const FString& FilePath);
	FString ConvertScanDataToJSON();
	bool ParseJSONToScanData(const FString& JSONString);

	// Playback
	void UpdatePlayback(float DeltaTime);
	void ApplyScanDataPoint(const FScanDataPoint& DataPoint);
	
	// Audio feedback
	void UpdateAudioFeedback(float DeltaTime);
	void PlayScannerSound(USoundBase* Sound);
	void StopScannerSound();
	
	// Text-to-Speech
	void SpeakText(const FString& TextToSpeak);
};
