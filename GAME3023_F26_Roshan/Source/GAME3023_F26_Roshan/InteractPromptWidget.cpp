// roshan - 101473963 - game3023 game engines 3
// interact prompt widget - minimal floating hud prompt with circular hold progress bar and square button logo in the center

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

		// 1. Filled circular dark background disc
		{
			const int32 NumCircleSegments = 48;
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			Vertices.Reserve(NumCircleSegments + 1);
			Indices.Reserve(NumCircleSegments * 3);

			const FLinearColor DiscLinearColor = (HoldProgress > 0.001f)
				? FLinearColor(0.10f, 0.13f, 0.20f, 0.94f * CombinedOpacity)
				: FLinearColor(0.06f, 0.07f, 0.11f, 0.88f * CombinedOpacity);
			const FColor DiscColor = DiscLinearColor.ToFColor(true);

			// Center vertex (index 0)
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform,
				FVector2f(Center),
				FVector2f(0.5f, 0.5f),
				DiscColor
			));

			for (int32 i = 0; i < NumCircleSegments; ++i)
			{
				const float Angle = (static_cast<float>(i) / static_cast<float>(NumCircleSegments)) * UE_TWO_PI;
				const FVector2D Pos = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * OuterRadius;
				Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					RenderTransform,
					FVector2f(Pos),
					FVector2f(0.5f, 0.5f),
					DiscColor
				));
			}

			for (int32 i = 0; i < NumCircleSegments; ++i)
			{
				const SlateIndex CurrentIdx = static_cast<SlateIndex>(i + 1);
				const SlateIndex NextIdx = static_cast<SlateIndex>(((i + 1) % NumCircleSegments) + 1);
				Indices.Add(0);
				Indices.Add(CurrentIdx);
				Indices.Add(NextIdx);
			}

			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId,
				ResourceHandle,
				Vertices,
				Indices,
				nullptr,
				0,
				0
			);
		}

		// 2. Circular background track ring (360 degrees)
		{
			const int32 NumTrackSegments = 64;
			TArray<FVector2f> TrackPoints;
			TrackPoints.Reserve(NumTrackSegments + 1);

			for (int32 i = 0; i <= NumTrackSegments; ++i)
			{
				const float Angle = -UE_HALF_PI + (static_cast<float>(i) / static_cast<float>(NumTrackSegments)) * UE_TWO_PI;
				const FVector2D Pt = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RingRadius;
				TrackPoints.Add(FVector2f(Pt));
			}

			const FLinearColor TrackColor(0.22f, 0.27f, 0.36f, 0.85f * CombinedOpacity);
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 1,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(TrackPoints),
				ESlateDrawEffect::None,
				TrackColor,
				true,
				3.5f
			);
		}

		// 3. Circular Progress Fill Arc (fills clockwise from top 12 o'clock over 0 to 5 seconds)
		if (HoldProgress > 0.001f)
		{
			const int32 MaxArcSegments = 64;
			const int32 ActiveSegments = FMath::Max(2, FMath::CeilToInt(HoldProgress * static_cast<float>(MaxArcSegments)));
			const float TotalSweepAngle = HoldProgress * UE_TWO_PI;
			const float StartAngle = -UE_HALF_PI; // 12 o'clock top

			// Smooth color transition from electric cyan to bright neon green as it approaches 5 seconds
			const FLinearColor ArcLinearColor = FMath::Lerp(
				FLinearColor(0.0f, 0.88f, 1.0f, 1.0f * CombinedOpacity),
				FLinearColor(0.15f, 1.0f, 0.45f, 1.0f * CombinedOpacity),
				FMath::Pow(HoldProgress, 1.4f)
			);
			const FColor ArcColor = ArcLinearColor.ToFColor(true);

			// 3a. Solid annulus band via MakeCustomVerts for zero-gap thick progress ring
			const float InnerArcRadius = RingRadius - 2.3f;
			const float OuterArcRadius = RingRadius + 2.3f;
			TArray<FSlateVertex> ArcVertices;
			TArray<SlateIndex> ArcIndices;
			ArcVertices.Reserve((ActiveSegments + 1) * 2);
			ArcIndices.Reserve(ActiveSegments * 6);

			for (int32 i = 0; i <= ActiveSegments; ++i)
			{
				const float Alpha = static_cast<float>(i) / static_cast<float>(ActiveSegments);
				const float Angle = StartAngle + Alpha * TotalSweepAngle;
				const FVector2D Dir(FMath::Cos(Angle), FMath::Sin(Angle));

				const FVector2D InnerPt = Center + Dir * InnerArcRadius;
				const FVector2D OuterPt = Center + Dir * OuterArcRadius;

				ArcVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					RenderTransform,
					FVector2f(InnerPt),
					FVector2f(0.5f, 0.5f),
					ArcColor
				));
				ArcVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
					RenderTransform,
					FVector2f(OuterPt),
					FVector2f(0.5f, 0.5f),
					ArcColor
				));
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
				OutDrawElements,
				LayerId + 2,
				ResourceHandle,
				ArcVertices,
				ArcIndices,
				nullptr,
				0,
				0
			);

			// 3b. Anti-aliased line pass along the progress arc for crisp edges
			TArray<FVector2f> ArcLinePoints;
			ArcLinePoints.Reserve(ActiveSegments + 1);
			for (int32 i = 0; i <= ActiveSegments; ++i)
			{
				const float Alpha = static_cast<float>(i) / static_cast<float>(ActiveSegments);
				const float Angle = StartAngle + Alpha * TotalSweepAngle;
				const FVector2D Pt = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * RingRadius;
				ArcLinePoints.Add(FVector2f(Pt));
			}

			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId + 3,
				AllottedGeometry.ToPaintGeometry(),
				MoveTemp(ArcLinePoints),
				ESlateDrawEffect::None,
				ArcLinearColor,
				true,
				4.5f
			);
		}

		// 4. PlayStation Square Button Logo in the exact center of the circle
		{
			const float SquareHalfSize = (HoldProgress > 0.001f) ? 7.2f : 8.0f;
			const float SquareSize = SquareHalfSize * 2.0f;
			const float WallThickness = 2.6f;
			const FVector2D TopLeft = Center - FVector2D(SquareHalfSize, SquareHalfSize);

			// Classic PlayStation Square Pink/Magenta
			const FLinearColor SquareColor = (HoldProgress > 0.001f)
				? FLinearColor(1.0f, 0.50f, 0.80f, 1.0f * CombinedOpacity)
				: FLinearColor(1.0f, 0.32f, 0.65f, 1.0f * CombinedOpacity);

			// Top bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(SquareSize, WallThickness), FSlateLayoutTransform(TopLeft)),
				WhiteBrush,
				ESlateDrawEffect::None,
				SquareColor
			);
			// Bottom bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(SquareSize, WallThickness), FSlateLayoutTransform(TopLeft + FVector2D(0.0f, SquareSize - WallThickness))),
				WhiteBrush,
				ESlateDrawEffect::None,
				SquareColor
			);
			// Left bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(WallThickness, SquareSize), FSlateLayoutTransform(TopLeft)),
				WhiteBrush,
				ESlateDrawEffect::None,
				SquareColor
			);
			// Right bar
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 4,
				AllottedGeometry.ToPaintGeometry(FVector2D(WallThickness, SquareSize), FSlateLayoutTransform(TopLeft + FVector2D(SquareSize - WallThickness, 0.0f))),
				WhiteBrush,
				ESlateDrawEffect::None,
				SquareColor
			);
		}

		return LayerId + 5;
	}

private:
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

	// Smoothly interpolate alpha
	CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, InDeltaTime, 14.0f);
	SetRenderOpacity(CurrentAlpha);

	if (bIsPromptVisible)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);

		// If following target in screen space
		if (!bAnchorToScreenBottom && TargetActor.IsValid())
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				const FVector WorldPos = TargetActor->GetActorLocation() + WorldSpaceOffset;
				FVector2D ScreenPos;
				if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, false))
				{
					// Center the prompt above the actor
					SetRenderTranslation(ScreenPos - FVector2D(80.0f, 40.0f));
				}
			}
		}
		else
		{
			SetRenderTranslation(FVector2D::ZeroVector);
		}
	}
	else
	{
		if (CurrentAlpha <= 0.01f)
		{
			CurrentAlpha = 0.0f;
			SetRenderOpacity(0.0f);
			SetVisibility(ESlateVisibility::Collapsed);
		}
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

	if (ActionTextBlock.IsValid())
	{
		ActionTextBlock->SetText(ActionText);
	}
	if (TargetTextBlock.IsValid())
	{
		TargetTextBlock->SetText(TargetNameText);
		TargetTextBlock->SetVisibility(TargetNameText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}

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
	// Only defer to Super if a Widget Blueprint tree with root content actually exists
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
				// Minimal Layout: Circular UI with Square Button Logo in the center + radial progress bar + text
				SNew(SHorizontalBox)

				// 1. Circular UI with PlayStation Square Button Logo in Center & 0-5s Circular Progress Ring
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
				[
					SAssignNew(CircularHoldBadge, SCircularHoldBadge)
					.Size(54.0f)
				]

				// 2. Text (Action + Target details with clean drop shadow)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)

					// Action Title: "Hold Square to Attach Battery" / "Press Square to Collect Battery"
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

					// Target Subtitle: "Laser Gun (ID: 1001)"
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(TargetTextBlock, STextBlock)
						.Text(TargetNameText)
						.Font(TargetNameFont)
						.ColorAndOpacity(FLinearColor(0.0f, 0.88f, 1.0f, 0.95f)) // Subtle electric cyan
						.ShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.ShadowOffset(FVector2D(1.5f, 1.5f))
					]
				]
			]
		];
}
