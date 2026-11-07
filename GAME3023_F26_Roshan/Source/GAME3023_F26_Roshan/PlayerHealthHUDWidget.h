// roshan - 101473963 - game3023 game engines 3
// player health hud widget - sleek top-right hud displaying player health points

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthHUDWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class STextBlock;

/**
 * UPlayerHealthHUDWidget
 * Top-right minimalist HUD widget showing the player's health points,
 * animated damage lag bar, dynamic health color transitions, and vital icon badge.
 */
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UPlayerHealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer);

	/** Updates the health display with new current and max health values */
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void UpdateHealth(float NewCurrentHealth, float NewMaxHealth);

	/** Direct setter for health percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void SetHealthPercent(float InPercent);

	/** Current health points */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Max health points */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetMaxHealth() const { return MaxHealth; }

	/** Current health percent (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetHealthPercent() const { return HealthPercent; }

	/** Current health points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float CurrentHealth = 100.0f;

	/** Maximum health points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float MaxHealth = 100.0f;

	/** Health percentage 0.0 to 1.0 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Health HUD")
	float HealthPercent = 1.0f;

	/** Width of the health bar in Slate units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarWidth = 340.0f;

	/** Height of the health bar in Slate units */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarHeight = 24.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Blueprint hook when health is updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnHealthUpdated(float NewCurrentHealth, float NewMaxHealth, float NewPercent);

	/** Blueprint hook when player takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnPlayerDamaged(float DamageAmount);

private:
	/** Animated visual percentage for smooth health bar transitions */
	float DisplayedPercent = 1.0f;

	/** Damage lag ghost bar percentage (catches up more slowly) */
	float LaggedPercent = 1.0f;

	/** Pulsing timer for critical health state */
	float PulseTimer = 0.0f;

	/** Slate widget handles */
	TSharedPtr<SBox> HealthBarFillBox;
	TSharedPtr<SBox> HealthBarLagBox;
	TSharedPtr<SBorder> HealthBarFillBorder;
	TSharedPtr<STextBlock> HealthValueTextBlock;
	TSharedPtr<STextBlock> HealthLabelTextBlock;
	TSharedPtr<SBorder> HealthIconBorder;
	TSharedPtr<STextBlock> HealthIconTextBlock;

	void RefreshVisuals();
};
