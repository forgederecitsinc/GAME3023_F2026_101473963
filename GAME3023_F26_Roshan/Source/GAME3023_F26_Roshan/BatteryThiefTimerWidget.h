#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BatteryThiefTimerWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class STextBlock;

// Widget that displays the remaining time while a thief removes a battery.
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UBatteryThiefTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor: initializes the timer widget.
	UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer);

	// Updates the remaining time and progress percentage.
	UFUNCTION(BlueprintCallable, Category = "Battery Thief Timer")
	void UpdateTimer(float InRemainingTime, float InTotalDuration);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float RemainingTime = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float TotalDuration = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float ProgressPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float WidgetWidth = 190.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float ProgressBarHeight = 6.0f;

protected:
	// Runs once after the widget is created.
	virtual void NativeConstruct() override;

	// Runs every frame to animate the countdown bar.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Builds the text and progress bar controls.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Blueprint event broadcast after timer values change.
	UFUNCTION(BlueprintImplementableEvent, Category = "Battery Thief Timer")
	void OnTimerUpdated(float InRemainingTime, float InProgressPercent);

private:
	// Updates the displayed countdown text.
	void RefreshVisuals();

	float DisplayedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> FillBarBox;
	TSharedPtr<SBorder> FillBarBorder;
	TSharedPtr<STextBlock> TimeTextBlock;
	TSharedPtr<STextBlock> ActionLabelTextBlock;
};
