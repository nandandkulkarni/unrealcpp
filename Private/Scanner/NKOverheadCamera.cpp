// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKOverheadCamera.h"
#include "Camera/CameraComponent.h"

ANKOverheadCamera::ANKOverheadCamera()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create root component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	// Create camera component
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	
	// Configure camera to look straight down
	Camera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));  // Pitch=-90° = looking down
	
	// Set initial height offset (will be overridden by attachment position)
	float HeightOffsetCm = HeightOffsetMeters * 100.0f;
	SetActorRelativeLocation(FVector(0.0f, 0.0f, HeightOffsetCm));
	
	UE_LOG(LogTemp, Log, TEXT("ANKOverheadCamera: Created with default height offset of %.1fm"), HeightOffsetMeters);
}

void ANKOverheadCamera::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Log, TEXT("ANKOverheadCamera: BeginPlay - Ready at position %s"), 
		*GetActorLocation().ToString());
}
