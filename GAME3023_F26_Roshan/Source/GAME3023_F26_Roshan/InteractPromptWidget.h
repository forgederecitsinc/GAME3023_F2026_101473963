#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

class SWidget;
class SBox;
class STextBlock;
class SCircularHoldBadge;

UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInteractPromptWidget(const FObjectInitializer& ObjectInitializer);

	/** Display the interaction prompt for a target actor with specific action and target name */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
 // ShowPrompt: show prompt
	void ShowPrompt(AActor* InTargetActor, const FText& InActionText, const FText& InTargetName);

	/** Hide the prompt smoothly */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
 // HidePrompt: hide prompt
	void HidePrompt();

	/** Update text while already visible */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
 // UpdatePromptText: update prompt text
	void UpdatePromptText(const FText& InActionText, const FText& InTargetName);

	/** Update the 0.0 to 1.0 circular hold progress around the Square button logo */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
 // SetHoldProgress: set hold progress
	void SetHoldProgress(float InProgress);

	/** Query current 0.0 to 1.0 hold progress */
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
 // GetHoldProgress: get hold progress
	float GetHoldProgress() const { return HoldProgress; }

	/** Query whether the prompt is currently visible */
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
 // IsPromptVisible: is prompt visible
	bool IsPromptVisible() const { return bIsPromptVisible; }

	/** Current target actor */
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
 // GetTargetActor: get target actor
	AActor* GetTargetActor() const { return TargetActor.Get(); }

	/** Current action text (e.g. "PRESS SQUARE TO PICK UP") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FText ActionText;

	/** Current target name text (e.g. "Battery Capsule [ID: 1001]") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FText TargetNameText;

	/** Keyboard alternative hint text (e.g. "E") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FText KeyboardKeyText;

	/** Current 0..1 progress for circular hold indicator (0 = not holding, 1 = 5s complete) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction Prompt")
	float HoldProgress = 0.0f;

	/** Whether to dock at bottom center of screen (true) or follow target actor in screen space (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	bool bAnchorToScreenBottom = true;

	/** Offset above target actor when following in screen space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FVector WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);

protected:
 // NativeConstruct: native construct
	virtual void NativeConstruct() override;
 // Called every frame
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
 // RebuildWidget: rebuild widget
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Blueprint hook: override in UMG Widget Blueprint if you want custom animations or styling */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
 // OnShowPrompt: on show prompt
	void OnShowPrompt(const FText& InActionText, const FText& InTargetName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
 // OnHidePrompt: on hide prompt
	void OnHidePrompt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
 // OnHoldProgressChanged: on hold progress changed
	void OnHoldProgressChanged(float NewProgress);

	/** Slate bindings */
 // GetActionText: get action text
	FText GetActionText() const { return ActionText; }
 // GetTargetNameText: get target name text
	FText GetTargetNameText() const { return TargetNameText; }
 // GetKeyboardKeyText: get keyboard key text
	FText GetKeyboardKeyText() const { return KeyboardKeyText; }
 // GetPromptSlateVisibility: get prompt slate visibility
	EVisibility GetPromptSlateVisibility() const;
 // GetPromptRenderOpacity: get prompt render opacity
	TOptional<float> GetPromptRenderOpacity() const { return CurrentAlpha; }

private:
 // UpdatePromptLabels: update prompt labels
	void UpdatePromptLabels();

	TWeakObjectPtr<AActor> TargetActor;

	bool bIsPromptVisible = false;
	float CurrentAlpha = 0.0f;
	float TargetAlpha = 0.0f;

	TSharedPtr<SBox> PromptContainerBox;
	TSharedPtr<SCircularHoldBadge> CircularHoldBadge;
	TSharedPtr<STextBlock> ActionTextBlock;
	TSharedPtr<STextBlock> TargetTextBlock;
	TSharedPtr<STextBlock> KeyTextBlock;
};
