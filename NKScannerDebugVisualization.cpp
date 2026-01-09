// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// DEBUG VISUALIZATION
// ========================================================================

#include "NKScannerCameraActor.h"
#include "DrawDebugHelpers.h"

void ANKScannerCameraActor::DrawOrbitPath()
{
	if (!GetWorld() || CinematicOrbitRadius <= 0.0f)
	{
		return;
	}
	
	LogMessage(TEXT("DrawOrbitPath: Drawing orbit visualization..."), true);
	
	// Draw the circular orbit path
	DrawDebugCircle(
		GetWorld(),
		CinematicOrbitCenter,                    // Center
		CinematicOrbitRadius,                    // Radius
		64,                                      // Segments (smooth circle)
		FColor::Green,                           // Color
		true,                                    // Persistent
		DebugVisualsLifetime,                    // Lifetime
		0,                                       // Depth priority
		2.0f,                                    // Thickness
		FVector(0, 1, 0),                        // Y axis (horizontal circle)
		FVector(1, 0, 0),                        // X axis
		false                                    // Draw axis
	);
	
	// Draw a sphere at the orbit center (target center)
	DrawDebugSphere(
		GetWorld(),
		CinematicOrbitCenter,
		50.0f,                                   // Larger sphere for center
		16,
		FColor::Red,                             // Red for center point
		true,
		DebugVisualsLifetime
	);
	
	// Draw a vertical line showing the orbit height
	DrawDebugLine(
		GetWorld(),
		CinematicOrbitCenter,
		FVector(CinematicOrbitCenter.X, CinematicOrbitCenter.Y, CinematicOrbitHeight),
		FColor::Magenta,
		true,
		DebugVisualsLifetime,
		0,
		3.0f
	);
	
	// Draw text label at center
	if (CinematicTargetLandscape)
	{
		DrawDebugString(
			GetWorld(),
			CinematicOrbitCenter + FVector(0, 0, 100),
			FString::Printf(TEXT("Target: %s\nRadius: %.0f cm\nHeight: %.0f cm"), 
				*CinematicTargetLandscape->GetName(),
				CinematicOrbitRadius,
				CinematicOrbitHeight),
			nullptr,
			FColor::White,
			DebugVisualsLifetime,
			true,  // Draw shadow
			1.5f   // Text scale
		);
	}
	
	LogMessage(FString::Printf(TEXT("DrawOrbitPath: Orbit circle drawn - Center: %s, Radius: %.2f"), 
		*CinematicOrbitCenter.ToString(), CinematicOrbitRadius), true);
}
