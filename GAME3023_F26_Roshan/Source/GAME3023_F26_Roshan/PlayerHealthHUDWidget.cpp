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

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);
	if (HealthBarFillBox.IsValid())
	{
		HealthBarFillBox->SetWidthOverride(FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth);
	}
	if (HealthBarLagBox.IsValid())
	{
		HealthBarLagBox->SetWidthOverride(FMath::Clamp(LaggedPercent, 0.0f, 1.0f) * UsableWidth);
	}

	FLinearColor BarColor(1.0f, 0.46f, 0.04f, 1.0f);
	if (HealthPercent <= 0.25f)
	{
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

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo IconFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, HealthBarWidth - 5.0f);
	const FLinearColor MetallicGreyBorder(0.58f, 0.63f, 0.68f, 0.95f);
	const FLinearColor DarkInteriorTrack(0.06f, 0.08f, 0.10f, 0.88f);
	const FLinearColor OrangeFillColor(1.0f, 0.46f, 0.04f, 1.0f);
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

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
					[
						SNew(SBox).WidthOverride(22.0f).HeightOverride(22.0f)
						[
							SAssignNew(HealthIconBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(MetallicGreyBorder)
							.HAlign(HAlign_Center).VAlign(VAlign_Center)
							.Padding(FMargin(1.8f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(DarkInteriorTrack)
								.HAlign(HAlign_Center).VAlign(VAlign_Center)
								[
									SAssignNew(HealthIconTextBlock, STextBlock)
									.Text(FText::FromString(TEXT("+")))
									.Font(IconFont)
									.ColorAndOpacity(OrangeFillColor)
								]
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(HealthLabelTextBlock, STextBlock)
						.Text(FText::FromString(TEXT("HP")))
						.Font(LabelFont)
						.ColorAndOpacity(FLinearColor(0.72f, 0.78f, 0.84f, 0.95f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.2f, 1.2f))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SAssignNew(HealthValueTextBlock, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth))))
						.Font(ValueFont)
						.ColorAndOpacity(FLinearColor::White)
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.5f, 1.5f))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox).WidthOverride(HealthBarWidth).HeightOverride(HealthBarHeight)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(MetallicGreyBorder)
						.Padding(FMargin(2.5f))
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
										SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(BurntOrangeLagColor)
									]
								]
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
								[
									SAssignNew(HealthBarFillBox, SBox).WidthOverride(UsableWidth)
									[
										SAssignNew(HealthBarFillBorder, SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(OrangeFillColor)
									]
								]
							]
						]
					]
				]
			]
		];
}
