// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKLaserTracerComponent.h"
#include "DrawDebugHelpers.h"
#include "CineCameraComponent.h"

UNKLaserTracerComponent::UNKLaserTracerComponent()
	: bLastShotHit(false)
	, LastHitActor(nullptr)
	, LastHitLocation(FVector::ZeroVector)
	, LastHitDistance(0.0f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UNKLaserTracerComponent::PerformTrace(FHitResult& OutHit)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	
	// Get camera component to trace from
	UCineCameraComponent* CineCamera = Owner->FindComponentByClass<UCineCameraComponent>();
	if (!CineCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKLaserTracerComponent: No CineCameraComponent found"));
		return false;
	}
	
	// Trace from camera forward
	FVector Start = CineCamera->GetComponentLocation();
	FVector End = Start + (CineCamera->GetForwardVector() * MaxRange);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		TraceChannel,
		QueryParams
	);
	
	// Update last shot state
	bLastShotHit = bHit;
	if (bHit)
	{
		LastHitActor = OutHit.GetActor();
		LastHitLocation = OutHit.Location;
		LastHitDistance = OutHit.Distance;
	}
	else
	{
		LastHitActor = nullptr;
		LastHitLocation = FVector::ZeroVector;
		LastHitDistance = 0.0f;
	}
	
	// Draw visualization
	if (bShowLaser)
	{
		DrawDiscoveryShot(Start, bHit ? OutHit.Location : End, bHit);
	}
	
	return bHit;
}

bool UNKLaserTracerComponent::PerformTraceAtAngle(float Angle, FHitResult& OutHit)
{
	// For now, just use regular trace (angle-based tracing would require rotating first)
	return PerformTrace(OutHit);
}

void UNKLaserTracerComponent::DrawLaserBeam(const FVector& Start, const FVector& End, bool bHit)
{
	if (!GetWorld())
	{
		return;
	}
	
	FColor Color = bHit ? FColor::Green : LaserColor;
	DrawDebugLine(GetWorld(), Start, End, Color, false, 0.1f, 0, LaserThickness);
	
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), End, 10.0f, 8, FColor::Yellow, false, 0.1f);
	}
}

void UNKLaserTracerComponent::DrawDiscoveryShot(const FVector& Start, const FVector& End, bool bHit)
{
	if (!GetWorld())
	{
		return;
	}
	
	// Color-coded persistent lines
	FColor Color = bHit ? FColor::Green : FColor::Red;
	float Thickness = bHit ? 3.0f : 1.0f;
	
	// Draw persistent line
	DrawDebugLine(GetWorld(), Start, End, Color, true, VisualsLifetime, 0, Thickness);
	
	// Draw sphere at hit point
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), End, 15.0f, 8, FColor::Yellow, true, VisualsLifetime);
	}
}

void UNKLaserTracerComponent::ClearLaserVisuals()
{
	if (GetWorld())
	{
		FlushPersistentDebugLines(GetWorld());
	}
}
