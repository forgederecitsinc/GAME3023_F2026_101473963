// roshan - 101473963 - game3023 game engines 3
// battery thief timer widget header - overhead timer UI while grabbing/removing battery

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BatteryThiefTimerWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class STextBlock;

/**
 * UBatteryThiefTimerWidget
 * Floating overhead UI widget displayed directly on top of the Battery Thief
 * while it is removing / grabbing a working battery from a laser device.
 * Displays a countdown timer, progress bar, and hazard status.
 */
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UBatteryThiefTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer);

	/** Updates the countdown timer and progress bar with current remaining seconds and total duration */
	UFUNCTION(BlueprintCallable, Category = "Battery Thief Timer")
	void UpdateTimer(float InRemainingTime, float InTotalDuration);

	/** Remaining countdown seconds */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float RemainingTime = 10.0f;

	/** Total duration of the removal process */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float TotalDuration = 10.0f;

	/** 0..1 remaining progress percentage (1 = full remaining time, 0 = time expired) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float ProgressPercent = 1.0f;

	/** Width of the timer widget in Slate units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float WidgetWidth = 190.0f;

	/** Height of the progress bar in Slate units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float ProgressBarHeight = 8.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Blueprint hook when timer is updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Battery Thief Timer")
	void OnTimerUpdated(float InRemainingTime, float InProgressPercent);

private:
	float DisplayedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> FillBarBox;
	TSharedPtr<SBorder> FillBarBorder;
	TSharedPtr<STextBlock> TimeTextBlock;
	TSharedPtr<STextBlock> ActionLabelTextBlock;
	TSharedPtr<SBorder> WarningIconBorder;
	TSharedPtr<STextBlock> WarningIconTextBlock;

	void RefreshVisuals();
};
