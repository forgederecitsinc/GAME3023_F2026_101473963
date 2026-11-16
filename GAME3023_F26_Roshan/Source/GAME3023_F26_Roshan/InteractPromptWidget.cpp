#include "InteractPromptWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"

// Constructor: sets up component hierarchy and default properties
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

// Initializes prompt badges, button icon textures, and text elements
void UInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.0f);
	CurrentAlpha = 0.0f;
	TargetAlpha = 0.0f;
}

// Smoothly fades widget in/out and updates hold progress circle
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

// UpdatePromptLabels: updates UI display and widget visibility
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

// Displays interaction prompt with specified action text and target name
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

// Fades out and hides interaction prompt widget
void UInteractPromptWidget::HidePrompt()
{
	bIsPromptVisible = false;
	TargetAlpha = 0.0f;
	CurrentAlpha = 0.0f;
	HoldProgress = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
	TargetActor.Reset();

	OnHidePrompt();
}

// UpdatePromptText: updates UI display and widget visibility
void UInteractPromptWidget::UpdatePromptText(const FText& InActionText, const FText& InTargetName)
{
	ActionText = InActionText;
	TargetNameText = InTargetName;
	UpdatePromptLabels();
}

// SetHoldProgress: handles set hold progress
void UInteractPromptWidget::SetHoldProgress(float InProgress)
{
	HoldProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	OnHoldProgressChanged(HoldProgress);
}

// GetPromptSlateVisibility: updates UI display and widget visibility
EVisibility UInteractPromptWidget::GetPromptSlateVisibility() const
{
	return (CurrentAlpha > 0.001f) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

// RebuildWidget: updates UI display and widget visibility
TSharedRef<SWidget> UInteractPromptWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	FSlateFontInfo ActionFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
	ActionFont.OutlineSettings.OutlineSize = 2;
	ActionFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	FSlateFontInfo TargetNameFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
	TargetNameFont.OutlineSettings.OutlineSize = 2;
	TargetNameFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	return SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 105.0f))
		[
			SAssignNew(PromptContainerBox, SBox)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
				[
					SAssignNew(ActionTextBlock, STextBlock)
					.Text(ActionText)
					.Font(ActionFont)
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SAssignNew(TargetTextBlock, STextBlock)
					.Text(TargetNameText)
					.Font(TargetNameFont)
					.ColorAndOpacity(FLinearColor::White)
					.Justification(ETextJustify::Center)
				]
			]
		];
}
