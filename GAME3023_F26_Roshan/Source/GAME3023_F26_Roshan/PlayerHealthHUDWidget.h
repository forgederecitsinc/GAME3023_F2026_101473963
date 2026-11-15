// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: PlayerHealthHUDWidget.h
// Description: Sleek top-right Player Health HUD widget.
// =============================================================================

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
 * Minimalist top-right HUD widget that displays:
 * - Current / Max Health numeric text (e.g. "100 / 100")
 * - Smooth orange health fill bar
 * - Delayed "damage lag" trail bar (gives game-feel visual feedback when hit)
 * - Pulsing warning effect when below 25% HP!
 */
UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UPlayerHealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor
	UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer);

	// =========================================================================
	// 🎮 PUBLIC API (Called by Character or GameMode to update health)
	// =========================================================================

	/** Call this whenever the player takes damage or heals to update the UI */
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void UpdateHealth(float NewCurrentHealth, float NewMaxHealth);

	/** Direct setter for health percentage (0.0 to 1.0) */
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void SetHealthPercent(float InPercent);

	/** Getter: Returns current health points */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetCurrentHealth() const { return CurrentHealth; }

	/** Getter: Returns max health points */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetMaxHealth() const { return MaxHealth; }

	/** Getter: Returns health percent (0.0 to 1.0) */
	UFUNCTION(BlueprintPure, Category = "Player Health HUD")
	float GetHealthPercent() const { return HealthPercent; }

	// =========================================================================
	// 🎮 TWEAKABLE HUD SETTINGS
	// =========================================================================

	// ✏️ CHANGE THIS: Current health points of the player (starts at 100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float CurrentHealth = 100.0f;

	// ✏️ CHANGE THIS: Maximum health points of the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float MaxHealth = 100.0f;

	// Health percentage 0.0 to 1.0 (read-only in inspector, calculated automatically)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player Health HUD")
	float HealthPercent = 1.0f;

	// ✏️ CHANGE THIS: Width of the health bar on screen in Slate units (default 340)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarWidth = 340.0f;

	// ✏️ CHANGE THIS: Height of the health bar on screen in Slate units (default 24)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Health HUD")
	float HealthBarHeight = 24.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// =========================================================================
	// 🔔 BLUEPRINT EVENTS (Override these in a Widget Blueprint if desired)
	// =========================================================================

	/** Blueprint hook triggered whenever health is updated */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnHealthUpdated(float NewCurrentHealth, float NewMaxHealth, float NewPercent);

	/** Blueprint hook triggered when player takes damage */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnPlayerDamaged(float DamageAmount);

private:
	// Smoothly animated percentage for the main health bar
	float DisplayedPercent = 1.0f;

	// Slower animated percentage for the damage catch-up ghost bar
	float LaggedPercent = 1.0f;

	// Timer tracking the pulse speed when health is critical (< 25%)
	float PulseTimer = 0.0f;

	// Internal Slate UI component pointers
	TSharedPtr<SBox> HealthBarFillBox;
	TSharedPtr<SBox> HealthBarLagBox;
	TSharedPtr<SBorder> HealthBarFillBorder;
	TSharedPtr<STextBlock> HealthValueTextBlock;
	TSharedPtr<STextBlock> HealthLabelTextBlock;
	TSharedPtr<SBorder> HealthIconBorder;
	TSharedPtr<STextBlock> HealthIconTextBlock;

	// Helper to refresh the numeric text label ("CurHP / MaxHP")
	void RefreshVisuals();
};
