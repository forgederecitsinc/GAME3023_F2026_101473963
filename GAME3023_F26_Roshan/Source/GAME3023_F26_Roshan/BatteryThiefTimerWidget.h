// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Floating overhead countdown widget shown while a Battery Thief removes a battery.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BatteryThiefTimerWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class STextBlock;

UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UBatteryThiefTimerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer);

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
	float ProgressBarHeight = 8.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Battery Thief Timer")
	void OnTimerUpdated(float InRemainingTime, float InProgressPercent);

private:
	void RefreshVisuals();

	float DisplayedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> FillBarBox;
	TSharedPtr<SBorder> FillBarBorder;
	TSharedPtr<STextBlock> TimeTextBlock;
	TSharedPtr<STextBlock> ActionLabelTextBlock;
	TSharedPtr<SBorder> WarningIconBorder;
	TSharedPtr<STextBlock> WarningIconTextBlock;
};
