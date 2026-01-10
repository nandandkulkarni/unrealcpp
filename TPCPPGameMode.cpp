// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPCPPGameMode.h"
#include "Scanner/UI/NKMappingScannerHUD.h"

ATPCPPGameMode::ATPCPPGameMode()
{
	// Set the HUD class to display scanner information
	HUDClass = ANKMappingScannerHUD::StaticClass();
}
