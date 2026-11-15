// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GAME3023_F26_RoshanPlayerController.cpp
// Description: Implementation of the Player Controller.
// Sets up Enhanced Input subsystems and applies Input Mapping Contexts.
// =============================================================================

#include "GAME3023_F26_RoshanPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "GAME3023_F26_Roshan.h"
#include "Widgets/Input/SVirtualJoystick.h"

// -----------------------------------------------------------------------------
// BEGIN PLAY: Spawns mobile/touch controls widget if running on touch platform
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Only spawn UI widgets on local player controllers (not dedicated servers)
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogGAME3023_F26_Roshan, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

// -----------------------------------------------------------------------------
// SETUP INPUT COMPONENT: Registers Enhanced Input Mapping Contexts (IMC)
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Only register inputs for the local player on this machine
	if (IsLocalPlayerController())
	{
		// Fetch the Enhanced Input Local Player Subsystem
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// 1. Add all default mapping contexts (WASD, Jump, Interact, Drop, etc.)
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// 2. Add contexts that are only enabled when NOT using touch controls
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// SHOULD USE TOUCH CONTROLS: Checks if device is mobile or touch is forced
// -----------------------------------------------------------------------------
bool AGAME3023_F26_RoshanPlayerController::ShouldUseTouchControls() const
{
	// Return true if running on mobile device or if bForceTouchControls is enabled in settings
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
