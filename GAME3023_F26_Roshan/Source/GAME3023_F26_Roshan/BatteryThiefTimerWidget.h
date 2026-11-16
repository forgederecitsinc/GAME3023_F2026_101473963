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
 // UpdateTimer: update timer
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
 // NativeConstruct: native construct
	virtual void NativeConstruct() override;
 // Called every frame
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
 // RebuildWidget: rebuild widget
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Battery Thief Timer")
 // OnTimerUpdated: on timer updated
	void OnTimerUpdated(float InRemainingTime, float InProgressPercent);

private:
 // RefreshVisuals: refresh visuals
	void RefreshVisuals();

	float DisplayedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> FillBarBox;
	TSharedPtr<SBorder> FillBarBorder;
	TSharedPtr<STextBlock> TimeTextBlock;
	TSharedPtr<STextBlock> ActionLabelTextBlock;
};
