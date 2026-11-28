#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthHUDWidget.generated.h"

class SWidget;
class SBox;
class SBorder;
class SHorizontalBox;
class STextBlock;
class SMinimalHeartWidget;

UCLASS(BlueprintType, Blueprintable)
class GAME3023_F26_ROSHAN_API UPlayerHealthHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Constructor: initializes the health HUD widget.
	UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer);

	// Updates the player's current and maximum health values.
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void UpdateHealth(float NewCurrentHealth, float NewMaxHealth);

	// Updates the number of lives shown by the heart icons.
	UFUNCTION(BlueprintCallable, Category = "Player Health HUD")
	void UpdateLives(int32 NewCurrentLives, int32 NewMaxLives);

	float CurrentHealth = 100.0f;

	float MaxHealth = 100.0f;

	float HealthPercent = 1.0f;

	float HealthBarWidth = 260.0f;

	float HealthBarHeight = 14.0f;

	int32 CurrentLives = 3;

	int32 MaxLives = 3;

protected:
	// Runs once after the widget is created.
	virtual void NativeConstruct() override;

	// Runs every frame to animate the health bar.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Creates the health bar, text, and heart layout.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	// Blueprint event called after health is updated.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnHealthUpdated(float NewCurrentHealth, float NewMaxHealth, float NewPercent);

	// Blueprint event called when the player takes damage.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnPlayerDamaged(float DamageAmount);

	// Blueprint event called after the life count changes.
	UFUNCTION(BlueprintImplementableEvent, Category = "Player Health HUD")
	void OnLivesUpdated(int32 NewCurrentLives, int32 NewMaxLives);

private:
	// Updates the health value text.
	void RefreshVisuals();

	// Rebuilds or updates the heart icons.
	void RefreshLivesVisuals();

	float DisplayedPercent = 1.0f;
	float LaggedPercent = 1.0f;
	float PulseTimer = 0.0f;

	TSharedPtr<SBox> HealthBarFillBox;
	TSharedPtr<SBox> HealthBarLagBox;
	TSharedPtr<SBorder> HealthBarFillBorder;
	TSharedPtr<STextBlock> HealthValueTextBlock;
	TSharedPtr<STextBlock> HealthLabelTextBlock;
	TSharedPtr<SHorizontalBox> HeartsContainerBox;
	TArray<TSharedPtr<SMinimalHeartWidget>> HeartWidgets;
};
