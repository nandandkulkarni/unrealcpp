// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// JSON PLAYBACK SYSTEM - Replay Recorded Camera Paths
// ========================================================================

#include "NKScannerCameraActor.h"

void ANKScannerCameraActor::StartJSONPlayback(FString JSONFilePath, float PlaybackSpeed, bool bLoop)
{
	LogMessage(FString::Printf(TEXT("StartJSONPlayback: Starting playback from: %s, Speed: %.2f, Loop: %s"),
		*JSONFilePath, PlaybackSpeed, bLoop ? TEXT("Yes") : TEXT("No")), true);

	if (!LoadScanDataFromJSON(JSONFilePath))
	{
		LogMessage(TEXT("StartJSONPlayback: ERROR - Failed to load scan data!"), true);
		return;
	}

	if (RecordedScanData.Num() == 0)
	{
		LogMessage(TEXT("StartJSONPlayback: ERROR - No scan data to playback!"), true);
		return;
	}

	bIsPlayingBack = true;
	CurrentPlaybackFrame = 0;
	PlaybackFrameAccumulator = 0.0f;
	PlaybackSpeedMultiplier = PlaybackSpeed;
	bLoopPlayback = bLoop;

	LogMessage(FString::Printf(TEXT("StartJSONPlayback: Playback started with %d frames"), RecordedScanData.Num()), true);
}

void ANKScannerCameraActor::StopPlayback()
{
	LogMessage(FString::Printf(TEXT("StopPlayback: Stopping playback at frame %d of %d"),
		CurrentPlaybackFrame, RecordedScanData.Num()), true);

	bIsPlayingBack = false;
	CurrentPlaybackFrame = 0;
	PlaybackFrameAccumulator = 0.0f;
}

float ANKScannerCameraActor::GetPlaybackProgress() const
{
	if (RecordedScanData.Num() == 0)
	{
		return 0.0f;
	}

	return static_cast<float>(CurrentPlaybackFrame) / static_cast<float>(RecordedScanData.Num());
}

void ANKScannerCameraActor::UpdatePlayback(float DeltaTime)
{
	if (RecordedScanData.Num() == 0)
	{
		LogMessage(TEXT("UpdatePlayback: ERROR - No playback data!"), true);
		StopPlayback();
		return;
	}

	PlaybackFrameAccumulator += DeltaTime * PlaybackSpeedMultiplier;

	while (PlaybackFrameAccumulator >= 0.0166f && bIsPlayingBack)
	{
		PlaybackFrameAccumulator -= 0.0166f;

		if (CurrentPlaybackFrame < RecordedScanData.Num())
		{
			ApplyScanDataPoint(RecordedScanData[CurrentPlaybackFrame]);
			CurrentPlaybackFrame++;

			if (bLogEveryFrame)
			{
				LogMessage(FString::Printf(TEXT("UpdatePlayback: Playing frame %d of %d"),
					CurrentPlaybackFrame, RecordedScanData.Num()));
			}
		}
		else
		{
			if (bLoopPlayback)
			{
				CurrentPlaybackFrame = 0;
				LogMessage(TEXT("UpdatePlayback: Looping playback"), true);
			}
			else
			{
				LogMessage(TEXT("UpdatePlayback: Playback completed"), true);
				StopPlayback();
			}
		}
	}
}

void ANKScannerCameraActor::ApplyScanDataPoint(const FScanDataPoint& DataPoint)
{
	SetActorLocation(DataPoint.CameraPosition);
	SetActorRotation(DataPoint.CameraRotation);

	LastHitLocation = DataPoint.LaserHitLocation;
	LastHitNormal = DataPoint.LaserHitNormal;
	LastHitDistance = DataPoint.HitDistance;

	if (bLogEveryFrame)
	{
		LogMessage(FString::Printf(TEXT("ApplyScanDataPoint: Applied frame %d - Pos: %s, Rot: %s"),
			DataPoint.FrameNumber, *DataPoint.CameraPosition.ToString(), *DataPoint.CameraRotation.ToString()));
	}
}
