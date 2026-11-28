#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

class SWidget;
class SBox;
class STextBlock;

// Widget that displays an interaction prompt for the player.
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor: initializes the widget.
	UInteractPromptWidget(const FObjectInitializer& ObjectInitializer);

	// Shows the prompt using the target actor and the text for the action and object.
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void ShowPrompt(AActor* InTargetActor, const FText& InActionText, const FText& InTargetName);

	// Starts hiding the prompt with a fade.
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void HidePrompt();

	// Changes the action and target text while the prompt is visible.
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void UpdatePromptText(const FText& InActionText, const FText& InTargetName);

	// Returns true while the prompt is visible.
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
	bool IsPromptVisible() const { return bIsPromptVisible; }

	// Returns the actor that the prompt is currently targeting.
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
	AActor* GetTargetActor() const { return TargetActor.Get(); }

	// Text telling the player what interaction to perform.
	FText ActionText;

	// Name of the object or actor the player can interact with.
	FText TargetNameText;

	// When true, the prompt stays at the bottom center of the screen.
	bool bAnchorToScreenBottom = true;

	// Position offset used when the prompt follows the target in the world.
	FVector WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);

protected:
	// Runs once after the widget is created.
	virtual void NativeConstruct() override;

	// Runs every frame to update fading and target position.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Builds the Slate controls shown by this widget.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Blueprint event that can be used to add custom show animations or styling.
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
	void OnShowPrompt(const FText& InActionText, const FText& InTargetName);

	// Blueprint event called when the prompt starts hiding.
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
	void OnHidePrompt();

private:
	// Updates the Slate text blocks with the latest prompt labels.
	void UpdatePromptLabels();

	// Weak reference to the actor the player can interact with.
	TWeakObjectPtr<AActor> TargetActor;

	// Tracks whether the prompt is currently visible.
	bool bIsPromptVisible = false;

	// Stores the current and target opacity used for fading.
	float CurrentAlpha = 0.0f;
	float TargetAlpha = 0.0f;

	// Slate controls that make up the interaction prompt.
	TSharedPtr<SBox> PromptContainerBox;
	TSharedPtr<STextBlock> ActionTextBlock;
	TSharedPtr<STextBlock> TargetTextBlock;
};
