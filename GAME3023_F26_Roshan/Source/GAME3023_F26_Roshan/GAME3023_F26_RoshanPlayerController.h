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

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	bool ShouldUseTouchControls() const;

};
