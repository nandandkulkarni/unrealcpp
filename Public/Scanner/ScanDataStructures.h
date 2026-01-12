// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScanDataStructures.generated.h"

/**
 * Mapping mode enum
 */
UENUM(BlueprintType)
enum class EMappingMode : uint8
{
	Orbit UMETA(DisplayName = "Orbit"),
	// Future: Grid, Adaptive, Spiral
};

/**
 * Orbit direction enum
 */
UENUM(BlueprintType)
enum class EOrbitDirection : uint8
{
	Clockwise UMETA(DisplayName = "Clockwise"),
	CounterClockwise UMETA(DisplayName = "Counter-Clockwise")
};

/**
 * Single scan data point
 */
USTRUCT(BlueprintType)
struct FScanDataPoint
{
	GENERATED_BODY()

	/** World position of hit */
	UPROPERTY(BlueprintReadOnly)
	FVector WorldPosition = FVector::ZeroVector;

	/** Surface normal at hit point */
	UPROPERTY(BlueprintReadOnly)
	FVector Normal = FVector::ZeroVector;

	/** Orbit angle when this point was captured (degrees) */
	UPROPERTY(BlueprintReadOnly)
	float OrbitAngle = 0.0f;

	/** Scan height (Z coordinate) */
	UPROPERTY(BlueprintReadOnly)
	float ScanHeight = 0.0f;

	/** Distance from camera to hit point */
	UPROPERTY(BlueprintReadOnly)
	float DistanceFromCamera = 0.0f;

	/** Actor that was hit */
	UPROPERTY(BlueprintReadOnly)
	AActor* HitActor = nullptr;

	/** Timestamp when captured (seconds since mapping started) */
	UPROPERTY(BlueprintReadOnly)
	float TimeStamp = 0.0f;

	/** Component that was hit */
	UPROPERTY(BlueprintReadOnly)
	FName ComponentName = NAME_None;
};
