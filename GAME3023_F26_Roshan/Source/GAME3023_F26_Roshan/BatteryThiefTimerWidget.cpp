#include "BatteryThiefTimerWidget.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

// Constructor: sets up component hierarchy and default properties
UBatteryThiefTimerWidget::UBatteryThiefTimerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVolatile = true;
}

// Sets up text blocks and progress circle bindings on widget spawn
void UBatteryThiefTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshVisuals();
}

// Updates countdown text and radial progress fill each frame
void UBatteryThiefTimerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, ProgressPercent, InDeltaTime, 14.0f);

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 3.0f);
	if (FillBarBox.IsValid())
	{
		FillBarBox->SetWidthOverride(FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth);
	}

	FLinearColor DynamicColor = FLinearColor::White;
	if (RemainingTime <= 2.5f)
	{
		PulseTimer += InDeltaTime * 10.0f;
		const float Pulse = 0.55f + 0.45f * FMath::Sin(PulseTimer);
		DynamicColor = FLinearColor(1.0f, 1.0f, 1.0f, Pulse);
	}
	else
	{
		PulseTimer = 0.0f;
	}

	if (FillBarBorder.IsValid())
	{
		FillBarBorder->SetBorderBackgroundColor(DynamicColor);
	}
	if (TimeTextBlock.IsValid())
	{
		TimeTextBlock->SetColorAndOpacity(DynamicColor);
	}
}

// UpdateTimer: handles update timer
void UBatteryThiefTimerWidget::UpdateTimer(float InRemainingTime, float InTotalDuration)
{
	RemainingTime = FMath::Max(0.0f, InRemainingTime);
	TotalDuration = FMath::Max(0.01f, InTotalDuration);
	ProgressPercent = FMath::Clamp(RemainingTime / TotalDuration, 0.0f, 1.0f);

	RefreshVisuals();
	OnTimerUpdated(RemainingTime, ProgressPercent);
}

// RefreshVisuals: handles refresh visuals
void UBatteryThiefTimerWidget::RefreshVisuals()
{
	if (TimeTextBlock.IsValid())
	{
		TimeTextBlock->SetText(FText::FromString(FString::Printf(TEXT("%.1fs"), FMath::Max(0.0f, RemainingTime))));
	}
}

// RebuildWidget: updates UI display and widget visibility
TSharedRef<SWidget> UBatteryThiefTimerWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	LabelFont.OutlineSettings.OutlineSize = 2;
	LabelFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);
	ValueFont.OutlineSettings.OutlineSize = 2;
	ValueFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 3.0f);
	const FLinearColor BlackOutlineColor = FLinearColor::Black;
	const FLinearColor DarkInteriorTrack(0.0f, 0.0f, 0.0f, 0.45f);
	const FLinearColor WhiteFillColor = FLinearColor::White;

	return SNew(SBox)
		.WidthOverride(WidgetWidth)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SAssignNew(ActionLabelTextBlock, STextBlock)
					.Text(FText::FromString(TEXT("REMOVING BATTERY")))
					.Font(LabelFont)
					.ColorAndOpacity(FLinearColor::White)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SAssignNew(TimeTextBlock, STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%.1fs"), RemainingTime)))
					.Font(ValueFont)
					.ColorAndOpacity(FLinearColor::White)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox).HeightOverride(ProgressBarHeight)
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
								SAssignNew(FillBarBox, SBox).WidthOverride(UsableWidth)
								[
									SAssignNew(FillBarBorder, SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(WhiteFillColor)
								]
							]
						]
					]
				]
			]
		];
}
