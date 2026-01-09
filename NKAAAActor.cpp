// Fill out your copyright notice in the Description page of Project Settings.


#include "NKAAAActor.h"

// Sets default values
ANKAAAActor::ANKAAAActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TestValue = 100.0f;
	ActorName = TEXT("MyTestActor");
}

// Called when the game starts or when spawned
void ANKAAAActor::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("NKAAAActor has been initialized!"));
}

// Called every frame
void ANKAAAActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

