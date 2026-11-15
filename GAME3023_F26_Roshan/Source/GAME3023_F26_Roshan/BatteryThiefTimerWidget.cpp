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

	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, ProgressPercent, InDeltaTime, 14.0f);

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 19.0f);
	if (FillBarBox.IsValid())
	{
		FillBarBox->SetWidthOverride(FMath::Clamp(DisplayedPercent, 0.0f, 1.0f) * UsableWidth);
	}

	FLinearColor DynamicColor(1.0f, 0.50f, 0.05f, 1.0f);
	if (RemainingTime <= 2.5f)
	{
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
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo IconFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	const float UsableWidth = FMath::Max(10.0f, WidgetWidth - 19.0f);
	const FLinearColor MetallicGreyBorder(0.55f, 0.60f, 0.65f, 0.95f);
	const FLinearColor DarkInteriorTrack(0.05f, 0.06f, 0.08f, 0.90f);
	const FLinearColor InitialColor(1.0f, 0.50f, 0.05f, 1.0f);

	return SNew(SBox)
		.WidthOverride(WidgetWidth)
		[
			SNew(SBorder)
			.BorderImage(WhiteBrush)
			.BorderBackgroundColor(MetallicGreyBorder)
			.Padding(FMargin(2.0f))
			[
				SNew(SBorder)
				.BorderImage(WhiteBrush)
				.BorderBackgroundColor(DarkInteriorTrack)
				.Padding(FMargin(6.0f, 4.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(FMargin(0.0f, 0.0f, 5.0f, 0.0f))
						[
							SNew(SBox).WidthOverride(16.0f).HeightOverride(16.0f)
							[
								SAssignNew(WarningIconBorder, SBorder)
								.BorderImage(WhiteBrush).BorderBackgroundColor(InitialColor)
								.HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(FMargin(1.0f))
								[
									SNew(SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(DarkInteriorTrack)
									.HAlign(HAlign_Center).VAlign(VAlign_Center)
									[
										SAssignNew(WarningIconTextBlock, STextBlock)
										.Text(FText::FromString(TEXT("!")))
										.Font(IconFont)
										.ColorAndOpacity(InitialColor)
									]
								]
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(ActionLabelTextBlock, STextBlock)
							.Text(FText::FromString(TEXT("REMOVING BATTERY")))
							.Font(LabelFont)
							.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.92f, 0.95f))
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
							.ShadowOffset(FVector2D(1.0f, 1.0f))
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNullWidget::NullWidget ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SAssignNew(TimeTextBlock, STextBlock)
							.Text(FText::FromString(FString::Printf(TEXT("%.1fs"), RemainingTime)))
							.Font(ValueFont)
							.ColorAndOpacity(InitialColor)
							.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
							.ShadowOffset(FVector2D(1.2f, 1.2f))
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox).HeightOverride(ProgressBarHeight)
						[
							SNew(SBorder)
							.BorderImage(WhiteBrush)
							.BorderBackgroundColor(FLinearColor(0.12f, 0.14f, 0.18f, 0.90f))
							.Padding(FMargin(0.0f))
							[
								SNew(SOverlay)
								+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Fill)
								[
									SAssignNew(FillBarBox, SBox).WidthOverride(UsableWidth)
									[
										SAssignNew(FillBarBorder, SBorder).BorderImage(WhiteBrush).BorderBackgroundColor(InitialColor)
									]
								]
							]
						]
					]
				]
			]
		];
}
