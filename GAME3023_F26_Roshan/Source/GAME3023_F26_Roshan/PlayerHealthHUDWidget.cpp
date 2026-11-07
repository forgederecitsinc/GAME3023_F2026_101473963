// roshan - 101473963 - game3023 game engines 3
// player health hud widget implementation

#include "PlayerHealthHUDWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

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
	bIsVolatile = true;
}

void UPlayerHealthHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisuals();
}

void UPlayerHealthHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Smoothly interpolate displayed fill towards target health percent
	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, HealthPercent, InDeltaTime, 14.0f);

	// Smoothly interpolate lag ghost bar more slowly for damage-trail feedback
	LaggedPercent = FMath::FInterpTo(LaggedPercent, HealthPercent, InDeltaTime, 3.5f);

	// Account for the 2.5px metallic border on each side
	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);
	const float CurrentFillW = FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth;
	const float LagFillW = FMath::Clamp(LaggedPercent, 0.0f, 1.0f) * UsableWidth;

	if (HealthBarFillBox.IsValid())
	{
		HealthBarFillBox->SetWidthOverride(CurrentFillW);
	}
	if (HealthBarLagBox.IsValid())
	{
		HealthBarLagBox->SetWidthOverride(LagFillW);
	}

	// Health fill color: Vibrant Orange as requested
	FLinearColor BarColor = FLinearColor(1.0f, 0.46f, 0.04f, 1.0f);

	if (HealthPercent <= 0.25f)
	{
		// Critical health: subtle pulse so player senses danger while retaining orange tone
		PulseTimer += InDeltaTime * 6.0f;
		const float Pulse = 0.65f + 0.35f * FMath::Sin(PulseTimer);
		BarColor = FLinearColor(1.0f, 0.38f, 0.02f, Pulse);
	}
	else
	{
		PulseTimer = 0.0f;
	}

	if (HealthBarFillBorder.IsValid())
	{
		HealthBarFillBorder->SetBorderBackgroundColor(BarColor);
	}
	if (HealthIconTextBlock.IsValid())
	{
		HealthIconTextBlock->SetColorAndOpacity(BarColor);
	}
}

void UPlayerHealthHUDWidget::UpdateHealth(float NewCurrentHealth, float NewMaxHealth)
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewCurrentHealth, 0.0f, FMath::Max(1.0f, NewMaxHealth));
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);

	if (OldHealth > CurrentHealth)
	{
		OnPlayerDamaged(OldHealth - CurrentHealth);
	}

	RefreshVisuals();
	OnHealthUpdated(CurrentHealth, MaxHealth, HealthPercent);
}

void UPlayerHealthHUDWidget::SetHealthPercent(float InPercent)
{
	UpdateHealth(InPercent * MaxHealth, MaxHealth);
}

void UPlayerHealthHUDWidget::RefreshVisuals()
{
	if (HealthValueTextBlock.IsValid())
	{
		const int32 CurHP = FMath::CeilToInt(CurrentHealth);
		const int32 MaxHP = FMath::CeilToInt(MaxHealth);
		HealthValueTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurHP, MaxHP)));
	}
}

TSharedRef<SWidget> UPlayerHealthHUDWidget::RebuildWidget()
{
	// Only defer to Super if a Widget Blueprint tree with root content actually exists
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo IconFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);

	// Metallic grey border color
	const FLinearColor MetallicGreyBorder(0.58f, 0.63f, 0.68f, 0.95f);
	// Dark interior background track
	const FLinearColor DarkInteriorTrack(0.06f, 0.08f, 0.10f, 0.88f);
	// Rich Orange fill color
	const FLinearColor OrangeFillColor(1.0f, 0.46f, 0.04f, 1.0f);
	// Dark burnt orange damage lag bar
	const FLinearColor BurntOrangeLagColor(0.55f, 0.22f, 0.04f, 0.90f);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 28.0f, 36.0f, 0.0f))
		[
			SNew(SBox)
			.WidthOverride(HealthBarWidth)
			[
				SNew(SVerticalBox)

				// Row 1: Minimal Header with Metallic Icon Badge, "HP" label, and large "100 / 100" text
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
				[
					SNew(SHorizontalBox)

					// Metallic / Vital Cross Icon Badge
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

					// "HP" Sub-label (Metallic steel tone)
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

					// Spacer pushing numeric counter to the right
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]

					// Numeric HP value: "100 / 100" (Large crisp typography)
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

				// Row 2: Health Bar Track with Metallic Border and Orange Fill
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(HealthBarWidth)
					.HeightOverride(HealthBarHeight)
					[
						// Metallic / Steel Grey Outer Border
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(MetallicGreyBorder)
						.Padding(FMargin(2.5f)) // 2.5px crisp metallic bezel
						[
							// Dark interior track
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(DarkInteriorTrack)
							.Padding(FMargin(0.0f))
							[
								SNew(SOverlay)

								// 1. Ghost/Lag Bar (shows damage catch-up trail in burnt orange)
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

								// 2. Main Health Fill Bar (Rich Orange)
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
