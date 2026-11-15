// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: BatteryThiefTimerWidget.h
// Description: Overhead floating countdown timer UI for the Battery Thief.
// Appears right above the Battery Thief while it is actively stealing/detaching
// a battery from a laser turret.
// =============================================================================

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
 * Displays:
 * - A countdown timer (e.g. "9.4s")
 * - A shrinking progress bar
 * - Warning icon that shifts color from Orange -> Amber -> Flashing Red!
 */
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UBatteryThiefTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor
	UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// 🎮 PUBLIC API (Called by EnemyBatteryThief to update countdown)
	// =========================================================================

	/** Updates the countdown timer and progress bar with current remaining seconds and total duration */
	UFUNCTION(BlueprintCallable, Category = "Battery Thief Timer")
	void UpdateTimer(float InRemainingTime, float InTotalDuration);

	// =========================================================================
	// 🎮 TWEAKABLE TIMER & SIZE SETTINGS
	// =========================================================================

	// Remaining countdown seconds (starts at 10.0 and counts down to 0.0)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float RemainingTime = 10.0f;

	// ✏️ CHANGE THIS: Total duration of the battery removal process in seconds (default 10.0s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float TotalDuration = 10.0f;

	// Progress percentage from 1.0 (full time) down to 0.0 (battery detached!)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery Thief Timer")
	float ProgressPercent = 1.0f;

	// ✏️ CHANGE THIS: Width of the floating timer widget on screen in Slate units (default 190)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float WidgetWidth = 190.0f;

	// ✏️ CHANGE THIS: Height of the progress bar in Slate units (default 8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief Timer")
	float ProgressBarHeight = 8.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// =========================================================================
	// 🔔 BLUEPRINT EVENTS
	// =========================================================================

	/** Blueprint hook triggered whenever the timer updates */
	UFUNCTION(BlueprintImplementableEvent, Category = "Battery Thief Timer")
	void OnTimerUpdated(float InRemainingTime, float InProgressPercent);

private:
	// Smoothly animated progress bar percentage
	float DisplayedPercent = 1.0f;

	// Flashing pulse timer for the critical countdown phase (< 2.5s remaining)
	float PulseTimer = 0.0f;

	// Internal Slate UI component pointers
	TSharedPtr<SBox> FillBarBox;
	TSharedPtr<SBorder> FillBarBorder;
	TSharedPtr<STextBlock> TimeTextBlock;
	TSharedPtr<STextBlock> ActionLabelTextBlock;
	TSharedPtr<SBorder> WarningIconBorder;
	TSharedPtr<STextBlock> WarningIconTextBlock;

	// Helper to refresh the numeric seconds text
	void RefreshVisuals();
};
