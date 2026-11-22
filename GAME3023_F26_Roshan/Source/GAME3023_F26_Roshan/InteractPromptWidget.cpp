#include "InteractPromptWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SOverlay.h"
#include "Styling/CoreStyle.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"

// Constructor: sets the starting text and prompt settings.
UInteractPromptWidget::UInteractPromptWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActionText = FText::FromString(TEXT("Press Square to Interact"));
	TargetNameText = FText::FromString(TEXT(""));
	HoldProgress = 0.0f;
	bAnchorToScreenBottom = true;
	WorldSpaceOffset = FVector(0.0f, 0.0f, 120.0f);
	bIsVolatile = true;
}

// NativeConstruct: runs after the widget is created and starts it in a hidden state.
void UInteractPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Hide the prompt until an object or actor needs to interact with the player.
	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.0f);
	CurrentAlpha = 0.0f;
	TargetAlpha = 0.0f;
}

// NativeTick: fades the widget and moves it near the target actor each frame.
void UInteractPromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Move the opacity smoothly toward the target opacity.
	CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, InDeltaTime, 14.0f);
	SetRenderOpacity(CurrentAlpha);

	if (!bIsPromptVisible)
	{
		// Once the prompt is almost transparent, fully collapse it.
		if (CurrentAlpha <= 0.01f)
		{
			CurrentAlpha = 0.0f;
			SetRenderOpacity(0.0f);
			SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	// Keep the prompt at the bottom center when it is anchored to the screen.
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

	// Find the target's position on the screen and move the prompt beside it.
	const FVector WorldPos = TargetActor->GetActorLocation() + WorldSpaceOffset;
	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(WorldPos, ScreenPos, false))
	{
		SetRenderTranslation(ScreenPos - FVector2D(80.0f, 40.0f));
	}
}

// UpdatePromptLabels: sends the newest action and target text to the UI blocks.
void UInteractPromptWidget::UpdatePromptLabels()
{
	if (ActionTextBlock.IsValid())
	{
		ActionTextBlock->SetText(ActionText);
	}
	if (TargetTextBlock.IsValid())
	{
		TargetTextBlock->SetText(TargetNameText);

		// Hide the target name line when no target name was provided.
		TargetTextBlock->SetVisibility(TargetNameText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
}

// ShowPrompt: displays the prompt for the selected target actor.
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

// HidePrompt: hides the prompt and clears the current target.
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

// UpdatePromptText: updates the prompt text without changing its target.
void UInteractPromptWidget::UpdatePromptText(const FText& InActionText, const FText& InTargetName)
{
	ActionText = InActionText;
	TargetNameText = InTargetName;
	UpdatePromptLabels();
}

// SetHoldProgress: limits the hold value and sends it to the Blueprint event.
void UInteractPromptWidget::SetHoldProgress(float InProgress)
{
	HoldProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	OnHoldProgressChanged(HoldProgress);
}

// RebuildWidget: creates the text and layout controls for the prompt.
TSharedRef<SWidget> UInteractPromptWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	// Create the font style for the main interaction action text.
	FSlateFontInfo ActionFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
	ActionFont.OutlineSettings.OutlineSize = 2;
	ActionFont.OutlineSettings.OutlineColor = FLinearColor::Black;

	// Create the font style for the smaller target name text.
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
