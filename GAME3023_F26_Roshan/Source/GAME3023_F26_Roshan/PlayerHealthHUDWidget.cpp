#include "PlayerHealthHUDWidget.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Blueprint/WidgetTree.h"

class SMinimalHeartWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMinimalHeartWidget)
		: _Size(18.0f)
		, _bFilled(true)
	{}
		SLATE_ARGUMENT(float, Size)
		SLATE_ARGUMENT(bool, bFilled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		HeartSize = InArgs._Size;
		bFilled = InArgs._bFilled;
	}

	void SetFilled(bool bInFilled)
	{
		bFilled = bInFilled;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(HeartSize, HeartSize);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		const float CombinedOpacity = InWidgetStyle.GetColorAndOpacityTint().A;
		if (CombinedOpacity <= 0.001f)
		{
			return LayerId;
		}

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const float Scale = FMath::Min(LocalSize.X, LocalSize.Y) / 36.0f;
		const FVector2D Center(LocalSize.X * 0.5f, LocalSize.Y * 0.5f - 1.5f * Scale);

		const int32 NumSegments = 36;
		TArray<FVector2f> OutlinePoints;
		OutlinePoints.Reserve(NumSegments + 1);

		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle ResourceHandle = WhiteBrush->GetRenderingResource();
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();

		const FLinearColor FillLinearColor = bFilled
			? FLinearColor(1.0f, 1.0f, 1.0f, CombinedOpacity)
			: FLinearColor(0.0f, 0.0f, 0.0f, 0.35f * CombinedOpacity);
		const FColor FillColor = FillLinearColor.ToFColor(true);

		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(NumSegments + 1);
		Indices.Reserve(NumSegments * 3);

		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform, FVector2f(Center), FVector2f(0.5f, 0.5f), FillColor));

		for (int32 i = 0; i <= NumSegments; ++i)
		{
			const float T = (static_cast<float>(i) / static_cast<float>(NumSegments)) * UE_TWO_PI;
			const float SinT = FMath::Sin(T);
			const float X = 16.0f * SinT * SinT * SinT;
			const float Y = -(13.0f * FMath::Cos(T) - 5.0f * FMath::Cos(2.0f * T) - 2.0f * FMath::Cos(3.0f * T) - FMath::Cos(4.0f * T));
			const FVector2D Pt = Center + FVector2D(X * Scale, Y * Scale);

			OutlinePoints.Add(FVector2f(Pt));
			if (i < NumSegments)
			{
				Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					RenderTransform, FVector2f(Pt), FVector2f(0.5f, 0.5f), FillColor));
			}
		}

		for (int32 i = 0; i < NumSegments; ++i)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(i + 1));
			Indices.Add(static_cast<SlateIndex>(((i + 1) % NumSegments) + 1));
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements, LayerId, ResourceHandle, Vertices, Indices, nullptr, 0, 0);

		const FLinearColor OutlineColor(0.0f, 0.0f, 0.0f, (bFilled ? 1.0f : 0.7f) * CombinedOpacity);
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(OutlinePoints), ESlateDrawEffect::None, OutlineColor, true, 1.8f);

		return LayerId + 2;
	}

private:
	float HeartSize = 18.0f;
	bool bFilled = true;
};

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
	RefreshLivesVisuals();
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

void UPlayerHealthHUDWidget::UpdateLives(int32 NewCurrentLives, int32 NewMaxLives)
{
	MaxLives = FMath::Max(1, NewMaxLives);
	CurrentLives = FMath::Clamp(NewCurrentLives, 0, MaxLives);
	RefreshLivesVisuals();
	OnLivesUpdated(CurrentLives, MaxLives);
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

void UPlayerHealthHUDWidget::RefreshLivesVisuals()
{
	if (!HeartsContainerBox.IsValid())
	{
		return;
	}

	if (HeartWidgets.Num() != MaxLives)
	{
		HeartsContainerBox->ClearChildren();
		HeartWidgets.Empty();

		for (int32 Index = 0; Index < MaxLives; ++Index)
		{
			TSharedPtr<SMinimalHeartWidget> HeartWidget;
			HeartsContainerBox->AddSlot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(Index > 0 ? 6.0f : 0.0f, 0.0f, 0.0f, 0.0f))
				[
					SAssignNew(HeartWidget, SMinimalHeartWidget)
					.Size(18.0f)
					.bFilled(Index < CurrentLives)
				];
			HeartWidgets.Add(HeartWidget);
		}
		return;
	}

	for (int32 Index = 0; Index < HeartWidgets.Num(); ++Index)
	{
		if (HeartWidgets[Index].IsValid())
		{
			HeartWidgets[Index]->SetFilled(Index < CurrentLives);
		}
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

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
				[
					SAssignNew(HeartsContainerBox, SHorizontalBox)
				]
			]
		];
}
