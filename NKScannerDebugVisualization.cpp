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
	
	// Draw text label at look-at target
	if (CinematicTargetLandscape)
	{
		DrawDebugString(
			GetWorld(),
			CinematicLookAtTarget + FVector(0, 0, 100),
			FString::Printf(TEXT("Target: %s\nHeight: %.0f%% (Z=%.0f cm)\nRadius: %.0f cm\nORBITS HORIZONTALLY"), 
				*CinematicTargetLandscape->GetActorLabel(),  // Use label instead of name
				CinematicHeightPercent,
				CinematicOrbitHeight,
				CinematicOrbitRadius),
			nullptr,
			FColor::White,
			DebugVisualsLifetime,
			true,  // Draw shadow
			1.5f   // Text scale
		);
		
		// ===== Draw Target Bounding Box =====
		if (bShowTargetBoundingBox)
		{
			FBox TargetBounds = CinematicTargetLandscape->GetComponentsBoundingBox(true);
			
			// Draw the bounding box
			DrawDebugBox(
				GetWorld(),
				TargetBounds.GetCenter(),
				TargetBounds.GetExtent(),
				BoundingBoxColor,
				true,  // Persistent
				DebugVisualsLifetime,
				0,
				3.0f  // Thickness
			);
			
			// Draw dimensions text at each corner
			FVector Min = TargetBounds.Min;
			FVector Max = TargetBounds.Max;
			FVector Size = TargetBounds.GetSize();
			
			// Draw size labels
			DrawDebugString(
				GetWorld(),
				FVector(Max.X, Max.Y, Max.Z + 50),
				FString::Printf(TEXT("Size: %.0fx%.0fx%.0f cm\n(%.1fx%.1fx%.1f m)"), 
					Size.X, Size.Y, Size.Z,
					Size.X/100.0f, Size.Y/100.0f, Size.Z/100.0f),
				nullptr,
				BoundingBoxColor,
				DebugVisualsLifetime,
				true,
				1.2f
			);
			
			// Draw min/max Z markers
			DrawDebugSphere(
				GetWorld(),
				FVector(TargetBounds.GetCenter().X, TargetBounds.GetCenter().Y, Min.Z),
				20.0f,
				8,
				FColor::Blue,  // Blue for bottom
				true,
				DebugVisualsLifetime
			);
			
			DrawDebugSphere(
				GetWorld(),
				FVector(TargetBounds.GetCenter().X, TargetBounds.GetCenter().Y, Max.Z),
				20.0f,
				8,
				FColor::Purple,  // Purple for top
				true,
				DebugVisualsLifetime
			);
			
			LogMessage(FString::Printf(TEXT("DrawOrbitPath: Bounding box drawn - Min: %s, Max: %s, Size: %s"), 
				*Min.ToString(), *Max.ToString(), *Size.ToString()), true);
		}
	}
	
	LogMessage(FString::Printf(TEXT("DrawOrbitPath: Horizontal orbit drawn - Center: %s, Radius: %.2f, Height: %.2f"), 
		*CinematicOrbitCenter.ToString(), CinematicOrbitRadius, CinematicOrbitHeight), true);
}
