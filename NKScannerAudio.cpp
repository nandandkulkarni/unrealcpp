// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// AUDIO FEEDBACK SYSTEM - Beeps & Text-to-Speech
// ========================================================================

#include "NKScannerCameraActor.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

void ANKScannerCameraActor::UpdateAudioFeedback(float DeltaTime)
{
	if (!ScannerAudioComponent)
	{
		return;
	}
	
	// Accumulate time since last sound played
	AudioTimeSinceLastPlay += DeltaTime;
	
	// Determine which sound to play based on current state
	USoundBase* SoundToPlay = nullptr;
	float CurrentInterval = 0.0f;
	
	switch (ScannerState)
	{
		case EScannerState::Validating:
			SoundToPlay = ValidationSound;
			CurrentInterval = ValidationSoundInterval;
			break;
			
		case EScannerState::Mapping:
			SoundToPlay = MappingSound;
			CurrentInterval = MappingSoundInterval;
			break;
			
		case EScannerState::Idle:
		case EScannerState::Complete:
		default:
			// No sound in idle or complete states
			StopScannerSound();
			return;
	}
	
	// State changed - reset timer
	if (LastAudioState != ScannerState)
	{
		AudioTimeSinceLastPlay = 0.0f;
		LastAudioState = ScannerState;
	}
	
	// Play sound at the appropriate interval
	if (SoundToPlay && AudioTimeSinceLastPlay >= CurrentInterval)
	{
		PlayScannerSound(SoundToPlay);
		AudioTimeSinceLastPlay = 0.0f;  // Reset timer
	}
}

void ANKScannerCameraActor::PlayScannerSound(USoundBase* Sound)
{
	if (!ScannerAudioComponent || !Sound)
	{
		return;
	}
	
	// Stop any currently playing sound
	if (ScannerAudioComponent->IsPlaying())
	{
		ScannerAudioComponent->Stop();
	}
	
	// Set the new sound and play it
	ScannerAudioComponent->SetSound(Sound);
	ScannerAudioComponent->SetVolumeMultiplier(AudioVolumeMultiplier);
	ScannerAudioComponent->Play();
	
	if (bLogEveryFrame)
	{
		LogMessage(FString::Printf(TEXT("Audio: Playing sound '%s' at volume %.2f"), 
			*Sound->GetName(), AudioVolumeMultiplier));
	}
}

void ANKScannerCameraActor::StopScannerSound()
{
	if (ScannerAudioComponent && ScannerAudioComponent->IsPlaying())
	{
		ScannerAudioComponent->Stop();
		LogMessage(TEXT("Audio: Stopped scanner sound"), true);
	}
}

// ========================================================================
// TEXT-TO-SPEECH FUNCTIONS
// ========================================================================

void ANKScannerCameraActor::SpeakText(const FString& TextToSpeak)
{
	if (!bEnableTextToSpeech || TextToSpeak.IsEmpty())
	{
		return;
	}
	
	// Note: This is a placeholder implementation
	// The actual TTS API in Unreal depends on which plugin you're using
	// You can implement this using:
	// 1. Platform-specific TTS (Windows SAPI, iOS AVSpeechSynthesizer, etc.)
	// 2. A third-party TTS plugin
	// 3. The Screen Reader plugin's TTS system
	
	LogMessage(FString::Printf(TEXT("TTS: [WOULD SPEAK] '%s' (at volume %.2f, rate %.2f)"),
		*TextToSpeak, TTSVolume, TTSRate), true);
	
	// TODO: Implement actual TTS when you determine which TTS system to use
	// Example pseudocode:
	// if (TTSSystem)
	// {
	//     TTSSystem->Speak(TextToSpeak, TTSVolume, TTSRate);
	// }
}
