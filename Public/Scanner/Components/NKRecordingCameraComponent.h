// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NKRecordingCameraComponent.generated.h"

/**
 * Recording camera mode - how camera looks at target
 */
UENUM(BlueprintType)
enum class ERecordingLookMode : uint8
{
	Perpendicular     UMETA(DisplayName = "Perpendicular to Orbit"),
	Center            UMETA(DisplayName = "Look at Orbit Center"),
	LookAhead         UMETA(DisplayName = "Look Ahead on Path")
};

/**
 * Smooth camera playback component for orbital mapping data
 * Implements Plan B-1: Tangent-based smooth camera movement
 * 
 * This component takes hit points from orbital mapping and creates smooth,
 * cinematic camera movement that glides along the orbit while maintaining
 * perpendicular viewing angles for complete surface coverage.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKRecordingCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNKRecordingCameraComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ===== Configuration =====
	
	/**
	 * Recording playback speed in meters per second
	 * Default: 0.3 m/s for slow, observable movement
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	float RecordingPlaybackSpeed = 0.3f;
	
	/**
	 * Recording camera offset distance from orbit (in cm)
	 * Default: 500cm = 5 meters
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	float RecordingOffsetDistanceCm = 500.0f;
	
	/**
	 * Recording camera look mode - how camera looks at target
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	ERecordingLookMode RecordingLookMode = ERecordingLookMode::Perpendicular;
	
	/**
	 * Recording look-ahead distance in cm (only used if RecordingLookMode = LookAhead)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback",
		meta = (EditCondition = "RecordingLookMode == ERecordingLookMode::LookAhead", EditConditionHides))
	float RecordingLookAheadDistanceCm = 100.0f;
	
	/**
	 * Loop recording playback continuously
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	bool bRecordingLoopPlayback = true;
	
	/**
	 * Enable detailed movement logging during playback
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	bool bRecordingEnableMovementLogging = false;
	
	/**
	 * Log interval in seconds (how often to log movement data)
	 * Default: 1.0 second
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback",
		meta = (EditCondition = "bRecordingEnableMovementLogging", EditConditionHides, ClampMin = "0.1", ClampMax = "10.0"))
	float RecordingMovementLogInterval = 1.0f;
	
	/**
	 * Target actor for recording camera (for Center look mode)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Playback")
	AActor* RecordingTargetActor;
	
	// ===== Debug Visualization =====
	
	/**
	 * Draw line from recording camera to orbit point
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Debug")
	bool bRecordingDrawDebugPath = true;
	
	/**
	 * Draw the orbital path during recording
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Debug")
	bool bRecordingDrawOrbitPath = true;
	
	/**
	 * Draw the recording camera path (offset orbit)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording Debug")
	bool bRecordingDrawCameraPath = true;
	
	// ===== Public API =====
	
	/**
	 * Start playback with given hit points from mapping
	 */
	UFUNCTION(BlueprintCallable, Category = "Recording Playback")
	void StartPlayback(const TArray<FVector>& InMappingHitPoints);
	
	/**
	 * Stop playback
	 */
	UFUNCTION(BlueprintCallable, Category = "Recording Playback")
	void StopPlayback();
	
	/**
	 * Pause/Resume playback
	 */
	UFUNCTION(BlueprintCallable, Category = "Recording Playback")
	void SetPaused(bool bPause);
	
	/**
	 * Get current playback progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Recording Playback")
	float GetProgress() const;
	
	/**
	 * Is currently playing?
	 */
	UFUNCTION(BlueprintPure, Category = "Recording Playback")
	bool IsPlaying() const { return bIsPlaying && !bIsPaused; }

private:
	// ===== Data =====
	
	/**
	 * Hit points from orbital mapping (circular orbit)
	 */
	UPROPERTY()
	TArray<FVector> MappingHitPoints;
	
	/**
	 * Total length of orbital path in cm
	 */
	float TotalPathLength = 0.0f;
	
	/**
	 * Current distance along path in cm
	 */
	float CurrentDistance = 0.0f;
	
	/**
	 * Orbit center point (calculated from hit points)
	 */
	FVector OrbitCenter = FVector::ZeroVector;
	
	/**
	 * Playback state
	 */
	bool bIsPlaying = false;
	bool bIsPaused = false;
	
	/**
	 * Movement logging timer
	 */
	float TimeSinceLastMovementLog = 0.0f;
	
	// ===== Helper Methods =====
	
	/**
	 * Calculate total path length from hit points
	 */
	float CalculateTotalPathLength() const;
	
	/**
	 * Calculate orbit center from hit points
	 */
	FVector CalculateOrbitCenter() const;
	
	/**
	 * Get interpolated position at distance along path
	 */
	FVector GetInterpolatedPosition(float DistanceAlongPath) const;
	
	/**
	 * Get tangent direction at distance along path
	 */
	FVector GetTangentAtDistance(float DistanceAlongPath) const;
	
	/**
	 * Calculate camera position from orbit point and tangent
	 */
	FVector CalculateCameraPosition(FVector OrbitPoint, FVector TangentDirection) const;
	
	/**
	 * Calculate camera rotation based on look mode
	 */
	FRotator CalculateCameraRotation(FVector CameraPosition, FVector OrbitPoint) const;
	
	/**
	 * Draw debug visualization
	 */
	void DrawDebugVisualization(FVector CameraPosition, FVector OrbitPoint);
};
