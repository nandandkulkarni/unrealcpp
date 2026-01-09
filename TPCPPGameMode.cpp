// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPCPPGameMode.h"
#include "NKScannerHUD.h"

ATPCPPGameMode::ATPCPPGameMode()
{
	// Set the HUD class to display scanner information
	HUDClass = ANKScannerHUD::StaticClass();
}
