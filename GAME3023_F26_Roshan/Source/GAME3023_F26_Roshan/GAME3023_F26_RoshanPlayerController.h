// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GAME3023_F26_RoshanPlayerController.h
// Description: Main Player Controller for the project.
// Manages Enhanced Input subsystems, Input Mapping Contexts (IMC), and
// optional touch / mobile controls.
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GAME3023_F26_RoshanPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 * AGAME3023_F26_RoshanPlayerController
 * PlayerController class for our 2.5D / Third-Person game.
 * Sets up Enhanced Input subsystems and applies default Input Mapping Contexts.
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	// -------------------------------------------------------------------------
	// 🎮 INPUT MAPPINGS (Enhanced Input Contexts)
	// -------------------------------------------------------------------------

	// Default Input Mapping Contexts (IMC_Default with WASD, Jump, Interact, Drop, etc.)
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	// Mapping contexts that should only be active when NOT on mobile/touch
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	// Mobile on-screen controls widget class (virtual joystick)
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	// Instance of the mobile controls widget
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	// ✏️ CHANGE THIS: Set to true if you want to test virtual touch controls on your desktop/Mac!
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	// -------------------------------------------------------------------------
	// 🧠 LIFECYCLE FUNCTIONS
	// -------------------------------------------------------------------------

	// Called on game start: initializes UI and controls
	virtual void BeginPlay() override;

	// Binds input components and applies IMCs to the local player subsystem
	virtual void SetupInputComponent() override;

	// Helper function: returns true if touch controls should be spawned
	bool ShouldUseTouchControls() const;

};
