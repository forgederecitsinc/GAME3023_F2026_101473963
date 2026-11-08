// roshan - 101473963 - game3023 game engines 3
// battery thief timer widget implementation

#include "BatteryThiefTimerWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

UBatteryThiefTimerWidget::UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, RemainingTime(10.0f)
	, TotalDuration(10.0f)
	, ProgressPercent(1.0f)
	, WidgetWidth(190.0f)
	, ProgressBarHeight(8.0f)
	, DisplayedPercent(1.0f)
	, PulseTimer(0.0f)
{
	bIsVolatile = true;
}

void UBatteryThiefTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisuals();
}

void UBatteryThiefTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Smoothly interpolate displayed percentage towards target progress
	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, ProgressPercent, InDeltaTime, 14.0f);

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 19.0f);
	const float FillWidth = FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth;

	if (FillBarBox.IsValid())
	{
		FillBarBox->SetWidthOverride(FillWidth);
	}

	// Dynamic color: Warning Orange -> Urgent Amber -> Flashing Crimson Red
	FLinearColor DynamicColor = FLinearColor(1.0f, 0.50f, 0.05f, 1.0f);

	if (RemainingTime <= 2.5f)
	{
		// Critical stage: rapid pulse indicating battery is about to be stolen
		PulseTimer += InDeltaTime * 10.0f;
		const float Pulse = 0.65f + 0.35f * FMath::Sin(PulseTimer);
		DynamicColor = FLinearColor(1.0f, 0.12f, 0.16f, Pulse);
	}
	else if (RemainingTime <= 5.0f)
	{
		DynamicColor = FLinearColor(1.0f, 0.30f, 0.08f, 1.0f);
	}
	else
	{
		PulseTimer = 0.0f;
	}

	if (FillBarBorder.IsValid())
	{
		FillBarBorder->SetBorderBackgroundColor(DynamicColor);
	}
	if (WarningIconBorder.IsValid())
	{
		WarningIconBorder->SetBorderBackgroundColor(DynamicColor);
	}
	if (WarningIconTextBlock.IsValid())
	{
		WarningIconTextBlock->SetColorAndOpacity(DynamicColor);
	}
	if (TimeTextBlock.IsValid())
	{
		TimeTextBlock->SetColorAndOpacity(DynamicColor);
	}
}

void UBatteryThiefTimerWidget::UpdateTimer(float InRemainingTime, float InTotalDuration)
{
	RemainingTime = FMath::Max(0.0f, InRemainingTime);
	TotalDuration = FMath::Max(0.01f, InTotalDuration);
	ProgressPercent = FMath::Clamp(RemainingTime / TotalDuration, 0.0f, 1.0f);

	RefreshVisuals();
	OnTimerUpdated(RemainingTime, ProgressPercent);
}

void UBatteryThiefTimerWidget::RefreshVisuals()
{
	if (TimeTextBlock.IsValid())
	{
		TimeTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), FMath::Max(0.0f, RemainingTime))));
	}
}

TSharedRef<SWidget> UBatteryThiefTimerWidget::RebuildWidget()
{
	// Only defer to Super if a Widget Blueprint tree with root content actually exists
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FSlateFontInfo TimeFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo IconFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 19.0f);

	return SNew(SBox)
		.WidthOverride(WidgetWidth)
		[
			// Outer dark translucent capsule with crisp metallic bezel
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.08f, 0.92f))
			.Padding(FMargin(8.0f, 6.0f, 8.0f, 6.0f))
			[
				SNew(SVerticalBox)

				// Row 1: Header row with warning icon, "STEALING" label, and countdown seconds
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
				[
					SNew(SHorizontalBox)

					// Warning Icon Badge: [!]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
					[
						SNew(SBox)
						.WidthOverride(16.0f)
						.HeightOverride(16.0f)
						[
							SAssignNew(WarningIconBorder, SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(1.0f, 0.50f, 0.05f, 1.0f))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.Padding(FMargin(1.0f))
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.09f, 0.95f))
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								[
									SAssignNew(WarningIconTextBlock, STextBlock)
									.Text(FText::FromString(TEXT("!")))
									.Font(IconFont)
									.ColorAndOpacity(FLinearColor(1.0f, 0.50f, 0.05f, 1.0f))
								]
							]
						]
					]

					// Action Label: "STEALING"
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(ActionLabelTextBlock, STextBlock)
						.Text(FText::FromString(TEXT("STEALING")))
						.Font(LabelFont)
						.ColorAndOpacity(FLinearColor(0.95f, 0.95f, 0.95f, 0.95f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.2f, 1.2f))
					]

					// Spacer
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNullWidget::NullWidget
					]

					// Countdown text: "10.0s"
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SAssignNew(TimeTextBlock, STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%.1fs"), RemainingTime)))
						.Font(TimeFont)
						.ColorAndOpacity(FLinearColor(1.0f, 0.50f, 0.05f, 1.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.2f, 1.2f))
					]
				]

				// Row 2: Progress bar track
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(ProgressBarHeight)
					[
						// Outer metallic grey bezel
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.45f, 0.50f, 0.55f, 0.90f))
						.Padding(FMargin(1.5f))
						[
							// Inner track
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.07f, 0.09f, 0.11f, 0.90f))
							.Padding(FMargin(0.0f))
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								.HAlign(HAlign_Left)
								.VAlign(VAlign_Fill)
								[
									SAssignNew(FillBarBox, SBox)
									.WidthOverride(UsableWidth)
									[
										SAssignNew(FillBarBorder, SBorder)
										.BorderImage(WhiteBrush)
										.BorderBackgroundColor(FLinearColor(1.0f, 0.50f, 0.05f, 1.0f))
									]
								]
							]
						]
					]
				]
			]
		];
}
