// roshan - 101473963 - game3023 game engines 3
// interact prompt widget - displays a sleek "Press [Square] to Interact" prompt
// with playstation square button glyph, optional keyboard hint [E], and context-aware action text

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractPromptWidget.generated.h"

class SWidget;
class SBox;
class STextBlock;

UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UInteractPromptWidget(const FObjectInitializer& ObjectInitializer);

	/** Display the interaction prompt for a target actor with specific action and target name */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void ShowPrompt(AActor* InTargetActor, const FText& InActionText, const FText& InTargetName);

	/** Hide the prompt smoothly */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void HidePrompt();

	/** Update text while already visible */
	UFUNCTION(BlueprintCallable, Category = "Interaction Prompt")
	void UpdatePromptText(const FText& InActionText, const FText& InTargetName);

	/** Query whether the prompt is currently visible */
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
	bool IsPromptVisible() const { return bIsPromptVisible; }

	/** Current target actor */
	UFUNCTION(BlueprintPure, Category = "Interaction Prompt")
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

	/** Whether to dock at bottom center of screen (true) or follow target actor in screen space (false) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	bool bAnchorToScreenBottom = true;

	/** Offset above target actor when following in screen space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FVector WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Blueprint hook: override in UMG Widget Blueprint if you want custom animations or styling */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
	void OnShowPrompt(const FText& InActionText, const FText& InTargetName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction Prompt")
	void OnHidePrompt();

	/** Slate bindings */
	FText GetActionText() const { return ActionText; }
	FText GetTargetNameText() const { return TargetNameText; }
	FText GetKeyboardKeyText() const { return KeyboardKeyText; }
	EVisibility GetPromptSlateVisibility() const;
	TOptional<float> GetPromptRenderOpacity() const { return CurrentAlpha; }

private:
	TWeakObjectPtr<AActor> TargetActor;

	bool bIsPromptVisible = false;
	float CurrentAlpha = 0.0f;
	float TargetAlpha = 0.0f;

	// Cached slate root box and text blocks
	TSharedPtr<SBox> PromptContainerBox;
	TSharedPtr<STextBlock> ActionTextBlock;
	TSharedPtr<STextBlock> TargetTextBlock;
	TSharedPtr<STextBlock> KeyTextBlock;
};
