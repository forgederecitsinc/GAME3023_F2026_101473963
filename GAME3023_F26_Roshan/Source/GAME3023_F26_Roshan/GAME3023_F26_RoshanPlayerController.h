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

 // Called when the game starts or when spawned
	virtual void BeginPlay() override;

 // SetupInputComponent: setup input component
	virtual void SetupInputComponent() override;

 // ShouldUseTouchControls: should use touch controls
	bool ShouldUseTouchControls() const;

};
