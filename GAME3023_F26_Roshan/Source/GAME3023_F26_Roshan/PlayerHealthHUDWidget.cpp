#include "PlayerHealthHUDWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

// Constructor: sets up component hierarchy and default properties
UPlayerHealthHUDWidget::UPlayerHealthHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVolatile = true;
}

// Binds health bar, damage vignette, and text elements
void UPlayerHealthHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisuals();
}

// Smoothly interpolates health bar fill and fades damage flashes
void UPlayerHealthHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, HealthPercent, InDeltaTime, 14.0f);
	LaggedPercent = FMath::FInterpTo(LaggedPercent, HealthPercent, InDeltaTime, 3.5f);

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 3.0f);
	if (HealthBarFillBox.IsValid())
	{
		HealthBarFillBox->SetWidthOverride(FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth);
	}
	if (HealthBarLagBox.IsValid())
	{
		HealthBarLagBox->SetWidthOverride(FMath::Clamp(LaggedPercent, 0.0f, 1.0f) * UsableWidth);
	}

	FLinearColor BarColor = FLinearColor::White;
	if (HealthPercent <= 0.25f)
	{
		PulseTimer += InDeltaTime * 6.0f;
		const float Pulse = 0.55f + 0.45f * FMath::Sin(PulseTimer);
		BarColor = FLinearColor(1.0f, 1.0f, 1.0f, Pulse);
	}
	else
	{
		PulseTimer = 0.0f;
	}

	if (HealthBarFillBorder.IsValid())
	{
		HealthBarFillBorder->SetBorderBackgroundColor(BarColor);
	}
}

// Updates current health and max health percentages
void UPlayerHealthHUDWidget::UpdateHealth(float NewCurrentHealth, float NewMaxHealth)
{
	const float OldHealth = CurrentHealth;
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = FMath::Clamp(NewCurrentHealth, 0.0f, MaxHealth);
	HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);

	if (OldHealth > CurrentHealth)
	{
		OnPlayerDamaged(OldHealth - CurrentHealth);
	}

	RefreshVisuals();
	OnHealthUpdated(CurrentHealth, MaxHealth, HealthPercent);
}

// SetHealthPercent: updates health and processes damage
void UPlayerHealthHUDWidget::SetHealthPercent(float InPercent)
{
	UpdateHealth(InPercent * MaxHealth, MaxHealth);
}

// RefreshVisuals: handles refresh visuals
void UPlayerHealthHUDWidget::RefreshVisuals()
{
	if (HealthValueTextBlock.IsValid())
	{
		HealthValueTextBlock->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth))));
	}
}

// RebuildWidget: updates UI display and widget visibility
TSharedRef<SWidget> UPlayerHealthHUDWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	LabelFont.OutlineSettings.OutlineSize = 2;
	LabelFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 15);
	ValueFont.OutlineSettings.OutlineSize = 2;
	ValueFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 3.0f);
	const FLinearColor BlackOutlineColor = FLinearColor::Black;
	const FLinearColor DarkInteriorTrack(0.0f, 0.0f, 0.0f, 0.45f);
	const FLinearColor WhiteFillColor = FLinearColor::White;
	const FLinearColor LagFillColor(1.0f, 1.0f, 1.0f, 0.35f);

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

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(HealthLabelTextBlock, STextBlock)
						.Text(FText::FromString(TEXT("HP")))
						.Font(LabelFont)
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(HealthValueTextBlock, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth))))
						.Font(ValueFont)
						.ColorAndOpacity(FLinearColor::White)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox).WidthOverride(HealthBarWidth).HeightOverride(HealthBarHeight)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(BlackOutlineColor)
						.Padding(FMargin(1.5f))
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(DarkInteriorTrack)
							.Padding(FMargin(0.0f))
							[
								SNew(SOverlay)
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
								[
									SAssignNew(HealthBarLagBox, SBox).WidthOverride(UsableWidth)
									[
										SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(LagFillColor)
									]
								]
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
								[
									SAssignNew(HealthBarFillBox, SBox).WidthOverride(UsableWidth)
									[
										SAssignNew(HealthBarFillBorder, SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(WhiteFillColor)
									]
								]
							]
						]
					]
				]
			]
		];
}
