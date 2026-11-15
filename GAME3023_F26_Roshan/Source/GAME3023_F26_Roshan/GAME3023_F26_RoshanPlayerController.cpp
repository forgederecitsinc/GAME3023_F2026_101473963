#include "GAME3023_F26_RoshanPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "GAME3023_F26_Roshan.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AGAME3023_F26_RoshanPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController() || !ShouldUseTouchControls())
	{
		return;
	}

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

void AGAME3023_F26_RoshanPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
	{
		Subsystem->AddMappingContext(CurrentContext, 0);
	}

	if (!ShouldUseTouchControls())
	{
		for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}

bool AGAME3023_F26_RoshanPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
