#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GAME3023_F26_RoshanPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 * Player controller for the 2.5D/third-person game.
 * It adds the default Enhanced Input mappings and optional mobile controls.
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	// Applies input mappings and creates the mobile controls after play begins.
	virtual void BeginPlay() override;

	// Registers Enhanced Input mappings for this local player.
	virtual void SetupInputComponent() override;

	// Returns true when mobile touch controls should be shown.
	bool ShouldUseTouchControls() const;

};
