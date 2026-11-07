// roshan - 101473963 - game3023 game engines 3
// interact prompt widget - minimal floating hud prompt (just logo + text, no background card)

#include "InteractPromptWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SSpacer.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"

UInteractPromptWidget::UInteractPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActionText = FText::FromString(TEXT("Press Square to Interact"));
	TargetNameText = FText::FromString(TEXT(""));
	KeyboardKeyText = FText::FromString(TEXT("E"));
	bAnchorToScreenBottom = true;
	WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);
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
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 105.0f))
		[
			SAssignNew(PromptContainerBox, SBox)
			[
				// Minimal Layout: Just the logo badge and clean text, NO background card
				SNew(SHorizontalBox)

				// 1. PlayStation Square Button Logo (Clean standalone button badge)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 10.0f, 0.0f))
				[
					SNew(SBox)
					.WidthOverride(28.0f)
					.HeightOverride(28.0f)
					[
						SNew(SBorder)
						.BorderImage(WhiteBrush)
						.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.12f, 0.85f))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.Padding(FMargin(4.0f))
						[
							// Classic PlayStation Square Symbol
							SNew(SBox)
							.WidthOverride(15.0f)
							.HeightOverride(15.0f)
							[
								SNew(SBorder)
								.BorderImage(WhiteBrush)
								.BorderBackgroundColor(FLinearColor(1.0f, 0.32f, 0.65f, 1.0f)) // PS Square Pink
								.Padding(FMargin(2.2f))
								[
									SNew(SBorder)
									.BorderImage(WhiteBrush)
									.BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.12f, 0.85f))
								]
							]
						]
					]
				]

				// 2. Text (Action + Target details with clean drop shadow)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)

					// Action Title: "Press Square to Interact"
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

					// Target Subtitle: "Battery Capsule (ID: 1001)"
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
