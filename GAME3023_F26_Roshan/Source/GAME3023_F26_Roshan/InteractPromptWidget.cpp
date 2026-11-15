#include "InteractPromptWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SSpacer.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"

/**
 * Custom Slate leaf widget that renders:
 * 1. A sleek circular dark badge disc
 * 2. A 360-degree circular progress track ring
 * 3. A clockwise circular progress fill arc (from 12 o'clock, 0.0 to 1.0 over 0 to 5 seconds)
 * 4. The PlayStation Square button logo centered inside the circle
 */
class SCircularHoldBadge : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCircularHoldBadge)
		: _Size(54.0f)
	{}
		SLATE_ARGUMENT(float, Size)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		BadgeSize = InArgs._Size;
		HoldProgress = 0.0f;
	}

	void SetHoldProgress(float InProgress)
	{
		HoldProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(BadgeSize, BadgeSize);
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
		const FVector2D Center = LocalSize * 0.5f;
		const float OuterRadius = FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f - 2.0f;
		const float RingRadius = OuterRadius - 3.5f;

		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle ResourceHandle = WhiteBrush->GetRenderingResource();
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();

		DrawBackgroundDisc(OutDrawElements, LayerId, ResourceHandle, RenderTransform, Center, OuterRadius, CombinedOpacity);
		DrawProgressTrack(OutDrawElements, LayerId + 1, AllottedGeometry, Center, RingRadius, CombinedOpacity);

		if (HoldProgress > 0.001f)
		{
			DrawProgressArc(OutDrawElements, LayerId + 2, AllottedGeometry, ResourceHandle, RenderTransform, Center, RingRadius, CombinedOpacity);
		}

		DrawSquareIcon(OutDrawElements, LayerId + 4, AllottedGeometry, WhiteBrush, Center, CombinedOpacity);

		return LayerId + 5;
	}

private:
	void DrawBackgroundDisc(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FSlateResourceHandle& ResourceHandle,
		const FSlateRenderTransform& RenderTransform,
		const FVector2D& Center,
		float OuterRadius,
		float Opacity) const
	{
		const int32 NumSegments = 48;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(NumSegments + 1);
		Indices.Reserve(NumSegments * 3);

		const FLinearColor DiscLinearColor = (HoldProgress > 0.001f)
			? FLinearColor(0.10f, 0.13f, 0.20f, 0.94f * Opacity)
			: FLinearColor(0.06f, 0.07f, 0.11f, 0.88f * Opacity);
		const FColor DiscColor = DiscLinearColor.ToFColor(true);

		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform, FVector2f(Center), FVector2f(0.5f, 0.5f), DiscColor));

		for (int32 i = 0; i < NumSegments; ++i)
		{
			const float Angle = (static_cast<float>(i) / static_cast<float>(NumSegments)) * UE_TWO_PI;
			const FVector2D Pos = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * OuterRadius;
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform, FVector2f(Pos), FVector2f(0.5f, 0.5f), DiscColor));
		}

		for (int32 i = 0; i < NumSegments; ++i)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(i + 1));
			Indices.Add(static_cast<SlateIndex>(((i + 1) % NumSegments) + 1));
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements, LayerId, ResourceHandle, Vertices, Indices, nullptr, 0, 0);
	}

	void DrawProgressTrack(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& AllottedGeometry,
		const FVector2D& Center,
		float RingRadius,
		float Opacity) const
	{
		const int32 NumSegments = 64;
		TArray<FVector2f> TrackPoints;
		TrackPoints.Reserve(NumSegments + 1);

		for (int32 i = 0; i <= NumSegments; ++i)
		{
			const float Angle = -UE_HALF_PI + (static_cast<float>(i) / static_cast<float>(NumSegments)) * UE_TWO_PI;
			const FVector2D Pt = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RingRadius;
			TrackPoints.Add(FVector2f(Pt));
		}

		const FLinearColor TrackColor(0.22f, 0.27f, 0.36f, 0.85f * Opacity);
		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(TrackPoints), ESlateDrawEffect::None, TrackColor, true, 3.5f);
	}

	void DrawProgressArc(
		FSlateWindowElementList& OutDrawElements,
		int32 BaseLayerId,
		const FGeometry& AllottedGeometry,
		const FSlateResourceHandle& ResourceHandle,
		const FSlateRenderTransform& RenderTransform,
		const FVector2D& Center,
		float RingRadius,
		float Opacity) const
	{
		const int32 MaxSegments = 64;
		const int32 ActiveSegments = FMath::Max(2, FMath::CeilToInt(HoldProgress * static_cast<float>(MaxSegments)));
		const float TotalSweep = HoldProgress * UE_TWO_PI;
		const float StartAngle = -UE_HALF_PI;

		const FLinearColor ArcLinearColor = FMath::Lerp(
			FLinearColor(0.0f, 0.88f, 1.0f, Opacity),
			FLinearColor(0.15f, 1.0f, 0.45f, Opacity),
			FMath::Pow(HoldProgress, 1.4f));
		const FColor ArcColor = ArcLinearColor.ToFColor(true);

		const float InnerRadius = RingRadius - 2.3f;
		const float OuterRadius = RingRadius + 2.3f;

		TArray<FSlateVertex> ArcVertices;
		TArray<SlateIndex> ArcIndices;
		TArray<FVector2f> ArcLinePoints;
		ArcVertices.Reserve((ActiveSegments + 1) * 2);
		ArcIndices.Reserve(ActiveSegments * 6);
		ArcLinePoints.Reserve(ActiveSegments + 1);

		for (int32 i = 0; i <= ActiveSegments; ++i)
		{
			const float Alpha = static_cast<float>(i) / static_cast<float>(ActiveSegments);
			const float Angle = StartAngle + Alpha * TotalSweep;
			const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));

			ArcVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform, FVector2f(Center + Dir * InnerRadius), FVector2f(0.5f, 0.5f), ArcColor));
			ArcVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform, FVector2f(Center + Dir * OuterRadius), FVector2f(0.5f, 0.5f), ArcColor));
			ArcLinePoints.Add(FVector2f(Center + Dir * RingRadius));
		}

		for (int32 i = 0; i < ActiveSegments; ++i)
		{
			const SlateIndex BaseIdx = static_cast<SlateIndex>(i * 2);
			ArcIndices.Add(BaseIdx);
			ArcIndices.Add(BaseIdx + 1);
			ArcIndices.Add(BaseIdx + 2);
			ArcIndices.Add(BaseIdx + 1);
			ArcIndices.Add(BaseIdx + 3);
			ArcIndices.Add(BaseIdx + 2);
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements, BaseLayerId, ResourceHandle, ArcVertices, ArcIndices, nullptr, 0, 0);
		FSlateDrawElement::MakeLines(
			OutDrawElements, BaseLayerId + 1, AllottedGeometry.ToPaintGeometry(),
			MoveTemp(ArcLinePoints), ESlateDrawEffect::None, ArcLinearColor, true, 4.5f);
	}

	void DrawSquareIcon(
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FGeometry& AllottedGeometry,
		const FSlateBrush* WhiteBrush,
		const FVector2D& Center,
		float Opacity) const
	{
		const float HalfSize = (HoldProgress > 0.001f) ? 7.2f : 8.0f;
		const float Size = HalfSize * 2.0f;
		const float Thickness = 2.6f;
		const FVector2D TopLeft = Center - FVector2D(HalfSize, HalfSize);

		const FLinearColor SquareColor = (HoldProgress > 0.001f)
			? FLinearColor(1.0f, 0.50f, 0.80f, Opacity)
			: FLinearColor(1.0f, 0.32f, 0.65f, Opacity);

		const TPair<FVector2D, FVector2D> Bars[4] = {
			{ TopLeft, FVector2D(Size, Thickness) },
			{ TopLeft + FVector2D(0.0f, Size - Thickness), FVector2D(Size, Thickness) },
			{ TopLeft, FVector2D(Thickness, Size) },
			{ TopLeft + FVector2D(Size - Thickness, 0.0f), FVector2D(Thickness, Size) }
		};

		for (const auto& Bar : Bars)
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(Bar.Value, FSlateLayoutTransform(Bar.Key)),
				WhiteBrush,
				ESlateDrawEffect::None,
				SquareColor);
		}
	}

	float BadgeSize = 54.0f;
	float HoldProgress = 0.0f;
};

UInteractPromptWidget::UInteractPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActionText = FText::FromString(TEXT("Press Square to Interact"));
	TargetNameText = FText::FromString(TEXT(""));
	KeyboardKeyText = FText::FromString(TEXT("E"));
	HoldProgress = 0.0f;
	bAnchorToScreenBottom = true;
	WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);
	bIsVolatile = true;
}

void UInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.0f);
	CurrentAlpha = 0.0f;
	TargetAlpha = 0.0f;
}

void UInteractPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, InDeltaTime, 14.0f);
	SetRenderOpacity(CurrentAlpha);

	if (!bIsPromptVisible)
	{
		if (CurrentAlpha <= 0.01f)
		{
			CurrentAlpha = 0.0f;
			SetRenderOpacity(0.0f);
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (bAnchorToScreenBottom || !TargetActor.IsValid())
	{
		SetRenderTranslation(FVector2D::ZeroVector);
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	const FVector WorldPos = TargetActor->GetActorLocation() + WorldSpaceOffset;
	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, false))
	{
		SetRenderTranslation(ScreenPos - FVector2D(80.0f, 40.0f));
	}
}

void UInteractPromptWidget::UpdatePromptLabels()
{
	if (ActionTextBlock.IsValid())
	{
		ActionTextBlock->SetText(ActionText);
	}
	if (TargetTextBlock.IsValid())
	{
		TargetTextBlock->SetText(TargetNameText);
		TargetTextBlock->SetVisibility(TargetNameText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
}

void UInteractPromptWidget::ShowPrompt(AActor* InTargetActor, const FText& InActionText, const FText& InTargetName)
{
	TargetActor = InTargetActor;
	ActionText = InActionText;
	TargetNameText = InTargetName;
	bIsPromptVisible = true;
	TargetAlpha = 1.0f;
	CurrentAlpha = 1.0f;

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.0f);
	UpdatePromptLabels();

	OnShowPrompt(InActionText, InTargetName);
}

void UInteractPromptWidget::HidePrompt()
{
	bIsPromptVisible = false;
	TargetAlpha = 0.0f;
	CurrentAlpha = 0.0f;
	HoldProgress = 0.0f;
	if (CircularHoldBadge.IsValid())
	{
		CircularHoldBadge->SetHoldProgress(0.0f);
	}
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
	TargetActor.Reset();

	OnHidePrompt();
}

void UInteractPromptWidget::UpdatePromptText(const FText& InActionText, const FText& InTargetName)
{
	ActionText = InActionText;
	TargetNameText = InTargetName;
	UpdatePromptLabels();
}

void UInteractPromptWidget::SetHoldProgress(float InProgress)
{
	HoldProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	if (CircularHoldBadge.IsValid())
	{
		CircularHoldBadge->SetHoldProgress(HoldProgress);
	}
	OnHoldProgressChanged(HoldProgress);
}

EVisibility UInteractPromptWidget::GetPromptSlateVisibility() const
{
	return (CurrentAlpha > 0.001f) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

TSharedRef<SWidget> UInteractPromptWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	const FSlateFontInfo ActionFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
	const FSlateFontInfo TargetNameFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 105.0f))
		[
			SAssignNew(PromptContainerBox, SBox)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
				[
					SAssignNew(CircularHoldBadge, SCircularHoldBadge)
					.Size(54.0f)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 1.0f))
					[
						SAssignNew(ActionTextBlock, STextBlock)
						.Text(ActionText)
						.Font(ActionFont)
						.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.5f, 1.5f))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(TargetTextBlock, STextBlock)
						.Text(TargetNameText)
						.Font(TargetNameFont)
						.ColorAndOpacity(FLinearColor(0.0f, 0.88f, 1.0f, 0.95f))
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.5f, 1.5f))
					]
				]
			]
		];
}
