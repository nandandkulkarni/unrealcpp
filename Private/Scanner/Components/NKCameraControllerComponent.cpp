// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKCameraControllerComponent.h"
#include "Scanner/Utilities/NKScannerLogger.h"
#include "CineCameraComponent.h"
#include "GameFramework/Actor.h"

UNKCameraControllerComponent::UNKCameraControllerComponent()
	: CineCameraComponent(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNKCameraControllerComponent::SetPosition(const FVector& Position)
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorLocation(Position);
		
		if (UNKScannerLogger* Logger = UNKScannerLogger::Get(this))
		{
			Logger->Log(FString::Printf(TEXT("Camera position set to: %s"), *Position.ToString()), TEXT("CameraController"));
		}
	}
}

void UNKCameraControllerComponent::MoveToOrbitPosition(float Angle, float Radius, const FVector& Center, float Height)
{
	FVector Position = CalculateOrbitPosition(Angle, Radius, Center, Height);
	SetPosition(Position);
	
	if (UNKScannerLogger* Logger = UNKScannerLogger::Get(this))
	{
		Logger->Log(FString::Printf(TEXT("Moved to orbit - Angle: %.1f°, Radius: %.2fm"), Angle, Radius/100.0f), TEXT("CameraController"));
	}
}

void UNKCameraControllerComponent::SetRotation(const FRotator& Rotation)
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorRotation(Rotation);
		
		if (UNKScannerLogger* Logger = UNKScannerLogger::Get(this))
		{
			Logger->Log(FString::Printf(TEXT("Rotation set - P:%.1f° Y:%.1f° R:%.1f°"), 
				Rotation.Pitch, Rotation.Yaw, Rotation.Roll), TEXT("CameraController"));
		}
	}
}

void UNKCameraControllerComponent::LookAtTarget(const FVector& Target)
{
	FVector CameraPos = GetCameraPosition();
	FRotator LookAtRot = CalculateLookAtRotation(CameraPos, Target);
	SetRotation(LookAtRot);
}

void UNKCameraControllerComponent::RotateToAngle(float Yaw)
{
	FRotator NewRotation(0.0f, Yaw, 0.0f);
	SetRotation(NewRotation);
}

FVector UNKCameraControllerComponent::CalculateOrbitPosition(float Angle, float Radius, const FVector& Center, float Height) const
{
	float AngleRadians = FMath::DegreesToRadians(Angle);
	
	FVector Position;
	Position.X = Center.X + (Radius * FMath::Cos(AngleRadians));
	Position.Y = Center.Y + (Radius * FMath::Sin(AngleRadians));
	Position.Z = Height;
	
	return Position;
}

FRotator UNKCameraControllerComponent::CalculateLookAtRotation(const FVector& CameraPos, const FVector& Target) const
{
	FVector Direction = Target - CameraPos;
	Direction.Normalize();
	return Direction.Rotation();
}

FVector UNKCameraControllerComponent::GetCameraPosition() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
}

FRotator UNKCameraControllerComponent::GetCameraRotation() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorRotation() : FRotator::ZeroRotator;
}

FVector UNKCameraControllerComponent::GetCameraForward() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;
}

UCineCameraComponent* UNKCameraControllerComponent::GetCineCameraComponent() const
{
	if (!CineCameraComponent)
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			const_cast<UNKCameraControllerComponent*>(this)->CineCameraComponent = 
				Owner->FindComponentByClass<UCineCameraComponent>();
		}
	}
	return CineCameraComponent;
}
