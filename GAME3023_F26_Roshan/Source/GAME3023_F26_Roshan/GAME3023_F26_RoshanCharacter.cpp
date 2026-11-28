#include "GAME3023_F26_RoshanCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DamageEvents.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GAME3023_F26_Roshan.h"
#include "BatteryCapsule.h"
#include "RedPowerCylinder.h"
#include "EnemyDrone.h"
#include "EnemyBatteryThief.h"
#include "InteractPromptWidget.h"
#include "PlayerHealthHUDWidget.h"
#include "Engine/OverlapResult.h"
#include "InputCoreTypes.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "UObject/ObjectKey.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// Constructor: creates the player capsule, camera boom, camera, and inventory components.
AGAME3023_F26_RoshanCharacter::AGAME3023_F26_RoshanCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 650.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 0.f;
	GetCharacterMovement()->AirControl = 0.0f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = bEnableCameraCollision;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->SocketOffset = CameraSocketOffset;

	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->CameraLagMaxDistance = 600.0f;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(CameraFOV);

	static ConstructorHelpers::FObjectFinder<USoundBase> BGMFinder(TEXT("/Game/Sound/LiveWire_Substation_Theme.LiveWire_Substation_Theme"));
	if (BGMFinder.Succeeded())
	{
		BackgroundMusicTrack = BGMFinder.Object;
	}

	MaxHealth = 100.0f;
	CurrentHealth = 100.0f;
	bIsDead = false;
}

// BeginPlay: initializes the HUD, camera settings, input, and background audio.
void AGAME3023_F26_RoshanCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyCinematicCameraSettings();

	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(true);
		GetMesh()->SetCustomDepthStencilValue(1);
	}

	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}

	if (IsLocallyControlled())
	{
		InitInteractionPromptWidget();
		InitHealthHUDWidget();
	}

	CurrentHealth = MaxHealth;
	RemainingLives = FMath::Max(1, MaxLives);
	bIsDead = false;
	bIsGameOver = false;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f);
	UpdateHealthHUD();

	if (bAutoPlayBGM && IsLocallyControlled())
	{
		PlayBackgroundMusic();
	}
}

// PossessedBy: runs after a controller takes control of the player character.
void AGAME3023_F26_RoshanCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AController* PC = NewController)
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}

	if (IsLocallyControlled())
	{
		InitInteractionPromptWidget();
		InitHealthHUDWidget();
		UpdateHealthHUD();

		if (bAutoPlayBGM && (!BGMAudioComponent || !BGMAudioComponent->IsPlaying()))
		{
			PlayBackgroundMusic();
		}
	}
}

// EndPlay: stops background audio and cleans up timers when play ends.
void AGAME3023_F26_RoshanCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopBackgroundMusic();

	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->RemoveFromParent();
		InteractionPromptWidget = nullptr;
	}

	if (HealthHUDWidget)
	{
		HealthHUDWidget->RemoveFromParent();
		HealthHUDWidget = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// Tick: updates interaction prompt each frame.
void AGAME3023_F26_RoshanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowInteractionWidget && IsLocallyControlled())
	{
		UpdateInteractionPrompt(DeltaTime);
	}
}

// OnConstruction: runs in editor when actor properties change
void AGAME3023_F26_RoshanCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCinematicCameraSettings();
}

#if WITH_EDITOR

// PostEditChangeProperty: refreshes editor view when properties are modified
void AGAME3023_F26_RoshanCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (CameraBoom)
	{
		if (PropName == GET_MEMBER_NAME_CHECKED(USpringArmComponent, TargetArmLength))
		{
			CameraDistance = CameraBoom->TargetArmLength;
		}
		else if (PropName == GET_MEMBER_NAME_CHECKED(USpringArmComponent, SocketOffset))
		{
			CameraSocketOffset = CameraBoom->SocketOffset;
		}
	}

	if (PropName == GET_MEMBER_NAME_CHECKED(AGAME3023_F26_RoshanCharacter, CameraDistance))
	{
		if (CameraBoom)
		{
			CameraBoom->TargetArmLength = CameraDistance;
		}
	}

	ApplyCinematicCameraSettings();
}
#endif

// ApplyCinematicCameraSettings: handles apply cinematic camera settings
void AGAME3023_F26_RoshanCharacter::ApplyCinematicCameraSettings()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 650.0f, 0.0f);
	}

	if (CameraBoom)
	{
		CameraBoom->SetUsingAbsoluteRotation(true);
		CameraBoom->SetRelativeLocation(FVector::ZeroVector);
		CameraBoom->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
		CameraBoom->TargetArmLength = CameraDistance;
		CameraBoom->SocketOffset = CameraSocketOffset;
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw = false;
		CameraBoom->bInheritRoll = false;
		CameraBoom->bDoCollisionTest = bEnableCameraCollision;
		CameraBoom->ProbeSize = 12.0f;
		CameraBoom->bEnableCameraLag = true;
		CameraBoom->CameraLagSpeed = CameraLagSpeed;
		CameraBoom->CameraLagMaxDistance = 600.0f;
		CameraBoom->bEnableCameraRotationLag = false;
	}

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(CameraFOV);
		FollowCamera->bUsePawnControlRotation = false;
		FollowCamera->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}
}

// AddControllerYawInput: handles add controller yaw input
void AGAME3023_F26_RoshanCharacter::AddControllerYawInput(float Val)
{
}

// AddControllerPitchInput: handles add controller pitch input
void AGAME3023_F26_RoshanCharacter::AddControllerPitchInput(float Val)
{
}

// AddControllerRollInput: handles add controller roll input
void AGAME3023_F26_RoshanCharacter::AddControllerRollInput(float Val)
{
}

// SetupPlayerInputComponent: binds movement, aiming, interaction, and drop actions.
void AGAME3023_F26_RoshanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		

		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Move);

		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Look);

		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
		}

		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
		}
	}
	else
	{
		UE_LOG(LogGAME3023_F26_Roshan, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}

	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);

	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);

	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
}

// Move: receives the player's movement input and sends it to the character.
void AGAME3023_F26_RoshanCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

// Look: receives the player's aiming input and updates the camera aim.
void AGAME3023_F26_RoshanCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

// DoMove: updates movement pathfinding and waypoint navigation
void AGAME3023_F26_RoshanCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FRotator YawRotation(0.0f, CameraYaw, 0.0f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

// DoLook: handles do look
void AGAME3023_F26_RoshanCharacter::DoLook(float Yaw, float Pitch)
{
}

// DoJumpStart: handles do jump start
void AGAME3023_F26_RoshanCharacter::DoJumpStart()
{
}

// DoJumpEnd: handles do jump end
void AGAME3023_F26_RoshanCharacter::DoJumpEnd()
{
}

// CanJumpInternal_Implementation: handles can jump internal implementation
bool AGAME3023_F26_RoshanCharacter::CanJumpInternal_Implementation() const
{
	return false;
}

// Jump: handles jump
void AGAME3023_F26_RoshanCharacter::Jump()
{
}

// StopJumping: handles stop jumping
void AGAME3023_F26_RoshanCharacter::StopJumping()
{
}

// InteractWithCylinder: inserts or removes a battery at the selected cylinder.
bool AGAME3023_F26_RoshanCharacter::InteractWithCylinder(ARedPowerCylinder* Cylinder)
{
	if (!Cylinder || !Cylinder->CanPlayerInteract(this))
	{
		return false;
	}

	Cylinder->Interact(this);
	return true;
}

// DoInteract: starts a pickup or cylinder interaction when the interact key is pressed.
void AGAME3023_F26_RoshanCharacter::DoInteract()
{
	if (bIsDead)
	{
		return;
	}

	if (CurrentInteractable)
	{
		if (ARedPowerCylinder* RedCell = Cast<ARedPowerCylinder>(CurrentInteractable))
		{
			InteractWithCylinder(RedCell);
			return;
		}
		if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(CurrentInteractable))
		{
			Battery->Interact(this);
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * 100.0f;
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (!World->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(180.0f), QueryParams))
	{
		return;
	}

	for (const FHitResult& Hit : HitResults)
	{
		if (ARedPowerCylinder* RedCell = Cast<ARedPowerCylinder>(Hit.GetActor()))
		{
			if (InteractWithCylinder(RedCell))
			{
				return;
			}
		}
	}

	for (const FHitResult& Hit : HitResults)
	{
		if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(Hit.GetActor()))
		{
			if (!Battery->bIsCarried && !Battery->bIsSocketed)
			{
				const FVector BatteryLoc = Battery->GetActorLocation();
				if (FVector::DistSquared2D(TraceStart, BatteryLoc) <= FMath::Square(Battery->BatteryPickupRadius) &&
					FMath::Abs(TraceStart.Z - BatteryLoc.Z) <= 160.0f)
				{
					Battery->Interact(this);
					return;
				}
			}
		}
	}
}

// TakeCarriedBattery: removes the battery from the player's hands and returns it.
ABatteryCapsule* AGAME3023_F26_RoshanCharacter::TakeCarriedBattery()
{
	if (!HasBattery()) return nullptr;

	ABatteryCapsule* Taken = CarriedBattery;
	CarriedBattery = nullptr;
	return Taken;
}

// TryPickupBattery: gives the battery to the player if the inventory hand is empty.
bool AGAME3023_F26_RoshanCharacter::TryPickupBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	if (HasBattery())
	{
		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] Inventory Full! Player is already carrying a Battery Capsule."));

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.5f,
				FColor::Red,
				TEXT("[INVENTORY FULL] You can only hold 1 Battery Capsule at a time!"),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		return false;
	}

	CarriedBattery = Battery;
	Battery->OnCollected(this);

	if (CurrentInteractable == Battery)
	{
		CurrentInteractable = nullptr;
	}

	UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Battery successfully picked up into inventory!"));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.5f,
			FColor::Green,
			TEXT("[BATTERY COLLECTED] Battery added to inventory (Slot: 1/1)."),
			true,
			FVector2D(1.3f, 1.3f)
		);
	}

	return true;
}

// DoDropBattery: manages battery attachment and state
void AGAME3023_F26_RoshanCharacter::DoDropBattery()
{
	if (HasBattery())
	{
		DropCurrentBattery();
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Silver,
				TEXT("[INVENTORY] No battery in inventory to drop.")
			);
		}
	}
}

// DropCurrentBattery: manages battery attachment and state
ABatteryCapsule* AGAME3023_F26_RoshanCharacter::DropCurrentBattery()
{
	if (!HasBattery()) return nullptr;

	ABatteryCapsule* Dropped = CarriedBattery;
	CarriedBattery = nullptr;

	const FVector ForwardOffset = GetActorForwardVector() * 120.0f;
	FVector DropLocation = GetActorLocation() + ForwardOffset;

	FHitResult FloorHit;
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	TraceParams.AddIgnoredActor(Dropped);

	if (GetWorld()->LineTraceSingleByChannel(FloorHit, DropLocation + FVector(0.f, 0.f, 50.f), DropLocation - FVector(0.f, 0.f, 300.f), ECC_WorldStatic, TraceParams))
	{
		DropLocation.Z = FloorHit.ImpactPoint.Z + 45.0f;
	}

	Dropped->OnDropped(DropLocation);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			TEXT("[BATTERY DROPPED] Battery dropped onto ground (Press E to pick up again)."),
			true,
			FVector2D(1.2f, 1.2f)
		);
	}

	return Dropped;
}

// Handles InitInteractionPromptWidget interaction logic
void AGAME3023_F26_RoshanCharacter::InitInteractionPromptWidget()
{
	if (InteractionPromptWidget)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	UClass* WidgetClassToUse = InteractionPromptWidgetClass ? InteractionPromptWidgetClass.Get() : UInteractPromptWidget::StaticClass();
	if (WidgetClassToUse)
	{
		InteractionPromptWidget = CreateWidget<UInteractPromptWidget>(PC, WidgetClassToUse);
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->AddToViewport(100);
			InteractionPromptWidget->HidePrompt();
			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Interaction Prompt Widget spawned successfully on Viewport."));
		}
	}
}

// InitHealthHUDWidget: updates health and processes damage
void AGAME3023_F26_RoshanCharacter::InitHealthHUDWidget()
{
	if (!bShowHealthHUD || HealthHUDWidget)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	UClass* WidgetClassToUse = HealthHUDWidgetClass ? HealthHUDWidgetClass.Get() : UPlayerHealthHUDWidget::StaticClass();
	if (WidgetClassToUse)
	{
		HealthHUDWidget = CreateWidget<UPlayerHealthHUDWidget>(PC, WidgetClassToUse);
		if (HealthHUDWidget)
		{
			HealthHUDWidget->AddToViewport(90);
			HealthHUDWidget->UpdateHealth(CurrentHealth, MaxHealth);
			HealthHUDWidget->UpdateLives(RemainingLives, MaxLives);
			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Player Health HUD Widget spawned successfully on Viewport (Top-Right)."));
		}
	}
}

// Updates health bar fill and status indicators on screen
void AGAME3023_F26_RoshanCharacter::UpdateHealthHUD()
{
	if (!HealthHUDWidget)
	{
		InitHealthHUDWidget();
	}
	if (HealthHUDWidget)
	{
		HealthHUDWidget->UpdateHealth(CurrentHealth, MaxHealth);
		HealthHUDWidget->UpdateLives(RemainingLives, MaxLives);
	}
}

// FindBestInteractable: finds the nearest battery or cylinder the player can use.
AActor* AGAME3023_F26_RoshanCharacter::FindBestInteractable()
{
	AActor* BestCandidate = nullptr;
	float BestDistSq = MAX_flt;
	const FVector PlayerLoc = GetActorLocation();

	auto ConsiderCandidate = [&](AActor* Candidate, float MaxDistSq)
	{
		if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(Candidate))
		{
			if (Battery->bIsCarried || Battery->bIsSocketed)
			{
				return false;
			}
			const FVector BatteryLoc = Battery->GetActorLocation();
			const float PickupLimitSq = FMath::Min(MaxDistSq, FMath::Square(Battery->BatteryPickupRadius));
			const float DistSq2D = FVector::DistSquared2D(PlayerLoc, BatteryLoc);
			if (DistSq2D <= PickupLimitSq && FMath::Abs(PlayerLoc.Z - BatteryLoc.Z) <= 160.0f)
			{
				const float DistSq = FVector::DistSquared(PlayerLoc, BatteryLoc);
				if (DistSq < BestDistSq)
				{
					BestCandidate = Battery;
					BestDistSq = DistSq;
				}
				return true;
			}
			return false;
		}
		if (ARedPowerCylinder* Cylinder = Cast<ARedPowerCylinder>(Candidate))
		{
			if (!Cylinder->CanPlayerInteract(this))
			{
				return false;
			}
			const float DistSq = FVector::DistSquared(PlayerLoc, Cylinder->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestCandidate = Cylinder;
				BestDistSq = DistSq;
			}
			return true;
		}
		return false;
	};

	if (CurrentInteractable && IsValid(CurrentInteractable))
	{
		if (!ConsiderCandidate(CurrentInteractable, FMath::Square(InteractionDetectionRadius + 60.0f)))
		{
			CurrentInteractable = nullptr;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return BestCandidate;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (CarriedBattery)
	{
		Params.AddIgnoredActor(CarriedBattery);
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	if (World->OverlapMultiByObjectType(Overlaps, PlayerLoc, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(InteractionDetectionRadius), Params))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* OverlapActor = Overlap.GetActor();
			if (OverlapActor && OverlapActor != this)
			{
				ConsiderCandidate(OverlapActor, MAX_flt);
			}
		}
	}

	return BestCandidate;
}

// Handles UpdateInteractionPrompt interaction logic
void AGAME3023_F26_RoshanCharacter::UpdateInteractionPrompt(float DeltaTime)
{
	if (!InteractionPromptWidget)
	{
		InitInteractionPromptWidget();
		if (!InteractionPromptWidget)
		{
			return;
		}
	}

	AActor* BestCandidate = FindBestInteractable();
	if (!BestCandidate)
	{
		CurrentInteractable = nullptr;
		InteractionPromptWidget->HidePrompt();
		return;
	}

	CurrentInteractable = BestCandidate;
	InteractionPromptWidget->bAnchorToScreenBottom = bAnchorPromptToScreenBottom;

	FString ActionStr = TEXT("Press Square to Interact");
	FString TargetStr;

	if (Cast<ABatteryCapsule>(BestCandidate))
	{
		ActionStr = HasBattery()
			? TEXT("Hands Full (Drop Battery First)")
			: TEXT("Press Square to Collect Battery");
		TargetStr = TEXT("Battery Capsule");
	}
	else if (ARedPowerCylinder* Cylinder = Cast<ARedPowerCylinder>(BestCandidate))
	{
		if (Cylinder->bHasBattery)
		{
			ActionStr = TEXT("Press Square to Collect Battery");
			TargetStr = TEXT("Laser Gun - Online");
		}
		else if (HasBattery())
		{
			ActionStr = TEXT("Press Square to Attach Battery");
			TargetStr = TEXT("Laser Gun - Ready");
		}
		else
		{
			ActionStr = TEXT("Laser Gun (Requires Battery)");
			TargetStr = TEXT("Laser Gun - Offline");
		}
	}

	InteractionPromptWidget->ShowPrompt(
		BestCandidate,
		FText::FromString(ActionStr),
		FText::FromString(TargetStr)
	);
}

// PlayBackgroundMusic: handles play background music
void AGAME3023_F26_RoshanCharacter::PlayBackgroundMusic()
{
	if (BGMAudioComponent && BGMAudioComponent->IsPlaying())
	{
		return;
	}

	if (!BackgroundMusicTrack)
	{
		BackgroundMusicTrack = LoadObject<USoundBase>(nullptr, TEXT("/Game/Sound/LiveWire_Substation_Theme.LiveWire_Substation_Theme"));
	}

	if (BackgroundMusicTrack)
	{
		if (USoundWave* Wave = Cast<USoundWave>(BackgroundMusicTrack))
		{
			Wave->bLooping = true;
		}

		BGMAudioComponent = UGameplayStatics::SpawnSound2D(
			this,
			BackgroundMusicTrack,
			BGMVolume,
			BGMPitch,
			0.0f,
			nullptr,
			false,
			false
		);

		if (BGMAudioComponent)
		{
			BGMAudioComponent->bAutoDestroy = false;
			BGMAudioComponent->OnAudioFinished.AddDynamic(this, &AGAME3023_F26_RoshanCharacter::OnBGMAudioFinished);

			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[LiveWire Audio] Background music started: %s (Volume: %.2f)"), *BackgroundMusicTrack->GetName(), BGMVolume);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, FString::Printf(TEXT("♪ LiveWire Theme Playing: %s"), *BackgroundMusicTrack->GetName()));
			}
		}
	}
	else
	{
		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[LiveWire Audio] Could not load /Game/Sound/LiveWire_Substation_Theme!"));
	}
}

// StopBackgroundMusic: handles stop background music
void AGAME3023_F26_RoshanCharacter::StopBackgroundMusic()
{
	if (BGMAudioComponent)
	{
		BGMAudioComponent->OnAudioFinished.RemoveAll(this);
		BGMAudioComponent->Stop();
		BGMAudioComponent = nullptr;
		UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[LiveWire Audio] Stopped background music."));
	}
}

// OnBGMAudioFinished: handles on bgm audio finished
void AGAME3023_F26_RoshanCharacter::OnBGMAudioFinished()
{
	if (bAutoPlayBGM && BGMAudioComponent)
	{
		BGMAudioComponent->Play();
	}
}

// TakeDamage: updates health and processes damage
float AGAME3023_F26_RoshanCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ApplyDamageToPlayer(ActualDamage, DamageCauser);
	return ActualDamage;
}

// ApplyDamage: updates health and processes damage
void AGAME3023_F26_RoshanCharacter::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (DamageImpulse.SizeSquared() > 0.0f && GetCharacterMovement())
	{
		GetCharacterMovement()->AddImpulse(DamageImpulse, true);
	}
	ApplyDamageToPlayer(Damage, DamageCauser);
}

// HandleDeath: handles elimination effects and actor cleanup
void AGAME3023_F26_RoshanCharacter::HandleDeath()
{
	bIsDead = true;
	RemainingLives = FMath::Max(0, RemainingLives - 1);
	UpdateHealthHUD();
	UE_LOG(LogTemplateCharacter, Warning, TEXT("[LiveWire Health] Player '%s' died! Remaining lives: %d/%d"),
		*GetNameSafe(this), RemainingLives, MaxLives);

	OnPlayerDied.Broadcast();

	if (HasBattery())
	{
		DropCurrentBattery();
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
	}

	if (RemainingLives <= 0)
	{
		HandleGameOver();
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			3.0f,
			FColor::Yellow,
			FString::Printf(TEXT("[LIFE LOST] %d %s remaining! Respawning..."),
				RemainingLives, RemainingLives == 1 ? TEXT("life") : TEXT("lives")),
			true,
			FVector2D(1.3f, 1.3f)
		);
	}

	if (UWorld* World = GetWorld())
	{
		if (RespawnDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				RespawnTimerHandle,
				this,
				&AGAME3023_F26_RoshanCharacter::RespawnPlayer,
				RespawnDelay,
				false
			);
		}
		else
		{
			RespawnPlayer();
		}
	}
}

void AGAME3023_F26_RoshanCharacter::HandleGameOver()
{
	if (bIsGameOver)
	{
		return;
	}

	bIsGameOver = true;
	OnGameOver.Broadcast();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	UE_LOG(LogTemplateCharacter, Error, TEXT("[LiveWire Health] GAME OVER - Player '%s' ran out of lives!"), *GetNameSafe(this));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Red,
			TEXT("[GAME OVER] Out of lives! Ending game..."),
			true,
			FVector2D(1.6f, 1.6f)
		);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
		APlayerController* PC = Cast<APlayerController>(GetController());
		auto QuitAction = [World, PC]()
		{
			if (IsValid(World))
			{
				UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
			}
		};

		if (GameOverQuitDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(GameOverTimerHandle, QuitAction, GameOverQuitDelay, false);
		}
		else
		{
			QuitAction();
		}
	}
}

// RespawnPlayer: handles spawning logic and placement
void AGAME3023_F26_RoshanCharacter::RespawnPlayer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	bIsDead = false;
	CurrentHealth = MaxHealth;
	UpdateHealthHUD();
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, MaxHealth);

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundDrones;
		UGameplayStatics::GetAllActorsOfClass(World, AEnemyDrone::StaticClass(), FoundDrones);
		for (AActor* Actor : FoundDrones)
		{
			if (AEnemyDrone* Drone = Cast<AEnemyDrone>(Actor))
			{
				if (Drone->IsAttachedTo(this) || Drone->DroneState != EEnemyDroneState::Patrolling)
				{
					Drone->ResetToStartAndResumePatrol();
				}
			}
		}

		TArray<AActor*> FoundThieves;
		UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), FoundThieves);
		for (AActor* Actor : FoundThieves)
		{
			if (AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(Actor))
			{
				if (Thief->IsFollowingPlayer())
				{
					Thief->StopFollowingPlayerAndResumePatrol();
				}
			}
		}
	}

	TeleportTo(RespawnLocation, GetActorRotation(), false, true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->Velocity = FVector::ZeroVector;
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	UE_LOG(LogTemplateCharacter, Log, TEXT("[LiveWire Health] Player '%s' respawned at %s with full health (%.0f HP)!"),
		*GetNameSafe(this), *RespawnLocation.ToString(), CurrentHealth);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			4.0f,
			FColor::Green,
			FString::Printf(TEXT("[RESPAWN] Player respawned at (X=%.0f, Y=%.0f, Z=%.0f) with %.0f HP!"),
				RespawnLocation.X, RespawnLocation.Y, RespawnLocation.Z, CurrentHealth),
			true,
			FVector2D(1.2f, 1.2f)
		);
	}
}

// ApplyHealing: handles apply healing
void AGAME3023_F26_RoshanCharacter::ApplyHealing(float Healing, AActor* Healer)
{
}

// NotifyDanger: handles notify danger
void AGAME3023_F26_RoshanCharacter::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
}

// UpdateHealthVisuals: refreshes health text and sends the current values to the HUD.
void AGAME3023_F26_RoshanCharacter::ApplyHealthChange(float NewHealth)
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	const float DeltaHealth = CurrentHealth - OldHealth;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, DeltaHealth);
	UpdateHealthHUD();
}

// ApplyDamageToPlayer: reduces health, updates the HUD, and starts death if health reaches zero.
void AGAME3023_F26_RoshanCharacter::ApplyDamageToPlayer(float DamageAmount, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return;
	}

	ApplyHealthChange(CurrentHealth - DamageAmount);
	UE_LOG(LogTemplateCharacter, Log, TEXT("[LiveWire Health] Player took %.1f damage from '%s'. Health: %.1f / %.1f"),
		DamageAmount, *GetNameSafe(DamageCauser), CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
}

