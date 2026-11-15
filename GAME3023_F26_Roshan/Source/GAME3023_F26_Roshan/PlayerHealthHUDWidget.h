// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Top-right Player Health HUD widget with smooth fill, damage lag trail, and low-HP pulse.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthHUDWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class STextBlock;

UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UPlayerHealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void UpdateHealth(float NewCurrentHealth, float NewMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void SetHealthPercent(float InPercent);

	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetHealthPercent() const { return HealthPercent; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Health HUD")
	float HealthPercent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarWidth = 340.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarHeight = 24.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnHealthUpdated(float NewCurrentHealth, float NewMaxHealth, float NewPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnPlayerDamaged(float DamageAmount);

private:
	void RefreshVisuals();

	float DisplayedPercent = 1.0f;
	float LaggedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> HealthBarFillBox;
	TSharedPtr<SBox> HealthBarLagBox;
	TSharedPtr<SBorder> HealthBarFillBorder;
	TSharedPtr<STextBlock> HealthValueTextBlock;
	TSharedPtr<STextBlock> HealthLabelTextBlock;
	TSharedPtr<SBorder> HealthIconBorder;
	TSharedPtr<STextBlock> HealthIconTextBlock;
};
