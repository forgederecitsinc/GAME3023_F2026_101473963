// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: PlayerHealthHUDWidget.cpp
// Description: Implementation of the Player Health HUD widget.
// Builds a custom Slate UI overlay in the top-right corner of the viewport,
// featuring a metallic bezel, orange fill, damage lag effect, and low-HP pulse.
// =============================================================================

#include "PlayerHealthHUDWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

// -----------------------------------------------------------------------------
// CONSTRUCTOR: Sets default values for health and bar sizing
// -----------------------------------------------------------------------------
UPlayerHealthHUDWidget::UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentHealth(100.0f)
	, MaxHealth(100.0f)
	, HealthPercent(1.0f)
	, HealthBarWidth(340.0f)
	, HealthBarHeight(24.0f)
	, DisplayedPercent(1.0f)
	, LaggedPercent(1.0f)
	, PulseTimer(0.0f)
{
	// Mark volatile so Slate updates the animation every frame
	bIsVolatile = true;
}

// -----------------------------------------------------------------------------
// NATIVE CONSTRUCT: Called when widget is added to viewport
// -----------------------------------------------------------------------------
void UPlayerHealthHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Pass-through mouse clicks so the HUD doesn't block player controls
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisuals();
}

// -----------------------------------------------------------------------------
// NATIVE TICK: Animates the smooth health bar fill, damage trail, and low-HP pulse
// -----------------------------------------------------------------------------
void UPlayerHealthHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. Smoothly interpolate displayed fill bar towards target percentage (fast snap)
	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, HealthPercent, InDeltaTime, 14.0f);

	// 2. Smoothly interpolate ghost lag bar more slowly for damage-trail feedback (slow catch-up)
	LaggedPercent = FMath::FInterpTo(LaggedPercent, HealthPercent, InDeltaTime, 3.5f);

	// Account for the 2.5px metallic border on each side to find usable width
	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);
	const float CurrentFillW = FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth;
	const float LagFillW = FMath::Clamp(LaggedPercent, 0.0f, 1.0f) * UsableWidth;

	// Update Slate box sizes to match new widths
	if (HealthBarFillBox.IsValid())
	{
		HealthBarFillBox->SetWidthOverride(CurrentFillW);
	}
	if (HealthBarLagBox.IsValid())
	{
		HealthBarLagBox->SetWidthOverride(LagFillW);
	}

	// 3. Health fill color: Vibrant Orange as requested by design
	// ✏️ CHANGE THIS: Change RGB values here if you want a different health bar color!
	FLinearColor BarColor = FLinearColor(1.0f, 0.46f, 0.04f, 1.0f);

	// 4. Critical health warning (< 25% HP): pulse opacity to alert the player
	if (HealthPercent <= 0.25f)
	{
		// ✏️ CHANGE THIS: Change 6.0f to pulse faster or slower when near death
		PulseTimer += InDeltaTime * 6.0f;
		const float Pulse = 0.65f + 0.35f * FMath::Sin(PulseTimer);
		BarColor = FLinearColor(1.0f, 0.38f, 0.02f, Pulse);
	}
	else
	{
		PulseTimer = 0.0f;
	}

	// Apply colors to borders and icon
	if (HealthBarFillBorder.IsValid())
	{
		HealthBarFillBorder->SetBorderBackgroundColor(BarColor);
	}
	if (HealthIconTextBlock.IsValid())
	{
		HealthIconTextBlock->SetColorAndOpacity(BarColor);
	}
}

// -----------------------------------------------------------------------------
// UPDATE HEALTH: Main function called when health points change
// -----------------------------------------------------------------------------
void UPlayerHealthHUDWidget::UpdateHealth(float NewCurrentHealth, float NewMaxHealth)
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewCurrentHealth, 0.0f, FMath::Max(1.0f, NewMaxHealth));
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);

	// If health dropped, trigger damage event hook
	if (OldHealth > CurrentHealth)
	{
		OnPlayerDamaged(OldHealth - CurrentHealth);
	}

	RefreshVisuals();
	OnHealthUpdated(CurrentHealth, MaxHealth, HealthPercent);
}

// -----------------------------------------------------------------------------
// SET HEALTH PERCENT: Helper to set health directly from a 0.0 - 1.0 ratio
// -----------------------------------------------------------------------------
void UPlayerHealthHUDWidget::SetHealthPercent(float InPercent)
{
	UpdateHealth(InPercent * MaxHealth, MaxHealth);
}

// -----------------------------------------------------------------------------
// REFRESH VISUALS: Updates the numeric "HP: Current / Max" text
// -----------------------------------------------------------------------------
void UPlayerHealthHUDWidget::RefreshVisuals()
{
	if (HealthValueTextBlock.IsValid())
	{
		const int32 CurHP = FMath::CeilToInt(CurrentHealth);
		const int32 MaxHP = FMath::CeilToInt(MaxHealth);
		HealthValueTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurHP, MaxHP)));
	}
}

// -----------------------------------------------------------------------------
// REBUILD WIDGET: Constructs the Slate hierarchy for the health bar
// -----------------------------------------------------------------------------
TSharedRef<SWidget> UPlayerHealthHUDWidget::RebuildWidget()
{
	// If a UMG Widget Blueprint tree exists, use that instead of code
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	// Font definitions
	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo IconFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);

	// ✏️ COLOR PALETTE: Change these colors to tweak HUD styling
	const FLinearColor MetallicGreyBorder(0.58f, 0.63f, 0.68f, 0.95f); // Metallic outer bezel
	const FLinearColor DarkInteriorTrack(0.06f, 0.08f, 0.10f, 0.88f);  // Background slot
	const FLinearColor OrangeFillColor(1.0f, 0.46f, 0.04f, 1.0f);        // Vibrant orange health fill
	const FLinearColor BurntOrangeLagColor(0.55f, 0.22f, 0.04f, 0.90f); // Damage lag trail

	// Build the top-right overlay
	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right) // Dock to right
		.VAlign(VAlign_Top)   // Dock to top
		.Padding(FMargin(0.0f, 28.0f, 36.0f, 0.0f)) // Distance from screen edges
		[
			SNew(SBox)
			.WidthOverride(HealthBarWidth)
			[
				SNew(SVerticalBox)

				// -------------------------------------------------------------
				// ROW 1: Minimal Header with [+] Icon, "HP" Label, and "100 / 100"
				// -------------------------------------------------------------
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
				[
					SNew(SHorizontalBox)

					// [+] Vital / Cross Icon Badge
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
					[
						SNew(SBox)
						.WidthOverride(22.0f)
						.HeightOverride(22.0f)
						[
							SAssignNew(HealthIconBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(MetallicGreyBorder)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FMargin(1.8f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(DarkInteriorTrack)
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SAssignNew(HealthIconTextBlock, STextBlock)
									.Text(FText::FromString(TEXT("+")))
									.Font(IconFont)
									.ColorAndOpacity(OrangeFillColor)
								]
							]
						]
					]

					// "HP" Text Label
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(HealthLabelTextBlock, STextBlock)
						.Text(FText::FromString(TEXT("HP")))
						.Font(LabelFont)
						.ColorAndOpacity(FLinearColor(0.72f, 0.78f, 0.84f, 0.95f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.2f, 1.2f))
					]

					// Spacer pushing numeric counter to far right
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]

					// Numeric HP value: "100 / 100"
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(HealthValueTextBlock, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth))))
						.Font(ValueFont)
						.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.5f, 1.5f))
					]
				]

				// -------------------------------------------------------------
				// ROW 2: The Health Bar with Metallic Border & Orange Fill
				// -------------------------------------------------------------
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(HealthBarWidth)
					.HeightOverride(HealthBarHeight)
					[
						// Outer metallic bezel border
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(MetallicGreyBorder)
						.Padding(FMargin(2.5f)) // 2.5px bezel thickness
						[
							// Dark background slot track
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(DarkInteriorTrack)
							.Padding(FMargin(0.0f))
							[
								SNew(SOverlay)

								// 1. Ghost damage trail bar (lags behind)
								+ SOverlay::Slot()
								.HAlign(HAlign_Left)
								.VAlign(VAlign_Fill)
								[
									SAssignNew(HealthBarLagBox, SBox)
									.WidthOverride(UsableWidth)
									[
										SNew(SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(BurntOrangeLagColor)
									]
								]

								// 2. Active Orange health fill bar
								+ SOverlay::Slot()
								.HAlign(HAlign_Left)
								.VAlign(VAlign_Fill)
								[
									SAssignNew(HealthBarFillBox, SBox)
									.WidthOverride(UsableWidth)
									[
										SAssignNew(HealthBarFillBorder, SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(OrangeFillColor)
									]
								]
							]
						]
					]
				]
			]
		];
}
