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

	SmoothedOcclusionTargetPos = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);

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

	for (auto& Pair : ActiveOccludingMeshes)
	{
		if (UPrimitiveComponent* Comp = Pair.Key)
		{
			const TArray<TObjectPtr<UMaterialInterface>>& OrigMats = Pair.Value.OriginalMaterials;
			for (int32 i = 0; i < OrigMats.Num(); ++i)
			{
				if (OrigMats[i])
				{
					Comp->SetMaterial(i, OrigMats[i]);
				}
			}
		}
	}
	ActiveOccludingMeshes.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// Tick: updates camera occlusion, aiming, hold interactions, and controller feedback.
void AGAME3023_F26_RoshanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableDualSenseController)
	{
		UpdateDualSenseControls(DeltaTime);
	}

	UpdateCameraOcclusionCutout(DeltaTime);

	if (bShowInteractionWidget && IsLocallyControlled())
	{
		UpdateInteractionPrompt(DeltaTime);
	}

	bMovedViaEnhancedInputThisFrame = false;
}

// UpdateDualSenseControls: handles update dual sense controls
void AGAME3023_F26_RoshanCharacter::UpdateDualSenseControls(float DeltaTime)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bMovedViaEnhancedInputThisFrame)
	{
		float StickX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
		float StickY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

		if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right)) StickX += 1.0f;
		if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left))  StickX -= 1.0f;
		if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up))    StickY += 1.0f;
		if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down))  StickY -= 1.0f;

		FVector2D StickVec(StickX, StickY);
		if (StickVec.Size() > ControllerStickDeadZone)
		{
			if (StickVec.SizeSquared() > 1.0f)
			{
				StickVec.Normalize();
			}
			DoMove(StickVec.X, StickVec.Y);
		}
	}

	if (!bEnableRightStickAiming)
	{
		return;
	}

	const float RightX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
	const float RawRightY = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY);
	const FVector2D AimVec(RightX, bInvertRightStickAimY ? -RawRightY : RawRightY);

	if (AimVec.Size() <= ControllerStickDeadZone)
	{
		if (GetCharacterMovement() && !GetCharacterMovement()->bOrientRotationToMovement)
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
		}
		return;
	}

	const FRotator CamYawRot(0.0f, CameraYaw, 0.0f);
	const FVector AimWorldDir =
		FRotationMatrix(CamYawRot).GetUnitAxis(EAxis::X) * AimVec.Y +
		FRotationMatrix(CamYawRot).GetUnitAxis(EAxis::Y) * AimVec.X;

	if (!AimWorldDir.IsNearlyZero())
	{
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}
		const FRotator TargetRot(0.0f, AimWorldDir.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 15.0f));
	}
}

// UpdateCameraOcclusionCutout: handles update camera occlusion cutout
void AGAME3023_F26_RoshanCharacter::UpdateCameraOcclusionCutout(float DeltaTime)
{
	if (!bEnableOcclusionCutout || !FollowCamera || !GetWorld())
	{
		return;
	}

	const FVector CameraLoc = FollowCamera->GetComponentLocation();
	const FVector PlayerTargetLoc = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
	SmoothedOcclusionTargetPos = FMath::VInterpTo(SmoothedOcclusionTargetPos, PlayerTargetLoc, DeltaTime, 18.0f);

	for (auto& Pair : ActiveOccludingMeshes)
	{
		Pair.Value.bHitThisFrame = false;
	}

	TArray<FHitResult> Hits;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CameraOcclusionCutout), false, this);
	if (CarriedBattery)
	{
		QueryParams.AddIgnoredActor(CarriedBattery);
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	const FCollisionShape SweepSphere = FCollisionShape::MakeSphere(35.0f);
	GetWorld()->SweepMultiByObjectType(Hits, CameraLoc, PlayerTargetLoc, FQuat::Identity, ObjectParams, SweepSphere, QueryParams);

	const float MinTopZ = GetActorLocation().Z + 40.0f;

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		AActor* HitActor = Hit.GetActor();
		if (!HitComp || !HitActor || HitActor == this || HitActor == CarriedBattery)
		{
			continue;
		}
		if (HitActor->IsA(ABatteryCapsule::StaticClass()) || HitActor->IsA(ARedPowerCylinder::StaticClass()))
		{
			continue;
		}
		if (HitComp->Bounds.GetBox().Max.Z < MinTopZ)
		{
			continue;
		}

		AcquireOccludingMesh(HitComp);
	}

	FadeOccludingMeshes(DeltaTime, CameraLoc);
}

// AcquireOccludingMesh: handles acquire occluding mesh
void AGAME3023_F26_RoshanCharacter::AcquireOccludingMesh(UPrimitiveComponent* Comp)
{
	if (FOccludingMeshData* ExistingData = ActiveOccludingMeshes.Find(Comp))
	{
		ExistingData->bHitThisFrame = true;
		return;
	}

	FOccludingMeshData NewData;
	NewData.CurrentAlpha = 0.0f;
	NewData.bHitThisFrame = true;

	const int32 NumMats = Comp->GetNumMaterials();
	NewData.OriginalMaterials.Reserve(NumMats);
	NewData.CutoutMIDs.Reserve(NumMats);

	for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
	{
		UMaterialInterface* OrigMat = Comp->GetMaterial(MatIdx);
		NewData.OriginalMaterials.Add(OrigMat);

		UMaterialInstanceDynamic* CutoutMID = nullptr;
		float StrengthCheck = 0.0f;
		if (OrigMat && OrigMat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("OcclusionStrength")), StrengthCheck))
		{
			CutoutMID = UMaterialInstanceDynamic::Create(OrigMat, this);
			if (CutoutMID)
			{
				Comp->SetMaterial(MatIdx, CutoutMID);
			}
		}
		NewData.CutoutMIDs.Add(CutoutMID);
	}

	ActiveOccludingMeshes.Add(Comp, MoveTemp(NewData));
}

// FadeOccludingMeshes: handles fade occluding meshes
void AGAME3023_F26_RoshanCharacter::FadeOccludingMeshes(float DeltaTime, const FVector& CameraLoc)
{
	const float SafeRadius = FMath::Max(OcclusionCutoutRadius, 20.0f);
	const float SafeSoftness = FMath::Clamp(OcclusionEdgeSoftness, 10.0f, SafeRadius * 0.9f);

	for (auto It = ActiveOccludingMeshes.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Comp = It.Key();
		FOccludingMeshData& Data = It.Value();

		if (!IsValid(Comp))
		{
			It.RemoveCurrent();
			continue;
		}

		const float TargetAlpha = Data.bHitThisFrame ? 1.0f : 0.0f;
		Data.CurrentAlpha = FMath::FInterpTo(Data.CurrentAlpha, TargetAlpha, DeltaTime, OcclusionFadeSpeed);

		for (UMaterialInstanceDynamic* MID : Data.CutoutMIDs)
		{
			if (MID)
			{
				MID->SetVectorParameterValue(TEXT("OcclusionCameraPos"), FLinearColor(CameraLoc.X, CameraLoc.Y, CameraLoc.Z, 1.0f));
				MID->SetVectorParameterValue(TEXT("OcclusionPlayerPos"), FLinearColor(SmoothedOcclusionTargetPos.X, SmoothedOcclusionTargetPos.Y, SmoothedOcclusionTargetPos.Z, 1.0f));
				MID->SetScalarParameterValue(TEXT("OcclusionRadius"), SafeRadius);
				MID->SetScalarParameterValue(TEXT("OcclusionSoftness"), SafeSoftness);
				MID->SetScalarParameterValue(TEXT("OcclusionStrength"), Data.CurrentAlpha);
				MID->SetScalarParameterValue(TEXT("OcclusionCenterOpacity"), OcclusionMinCenterOpacity);
				MID->SetVectorParameterValue(TEXT("OcclusionRimColor"), OcclusionRimColor);
				MID->SetScalarParameterValue(TEXT("OcclusionRimIntensity"), OcclusionRimIntensity);
			}
		}

		if (!Data.bHitThisFrame && Data.CurrentAlpha <= 0.01f)
		{
			for (int32 MatIdx = 0; MatIdx < Data.OriginalMaterials.Num(); ++MatIdx)
			{
				if (Data.OriginalMaterials[MatIdx])
				{
					Comp->SetMaterial(MatIdx, Data.OriginalMaterials[MatIdx]);
				}
			}
			It.RemoveCurrent();
		}
	}
}

// TriggerControllerRumble: handles trigger controller rumble
void AGAME3023_F26_RoshanCharacter::TriggerControllerRumble(float Intensity, float Duration, bool bHeavyLeft, bool bLightRight)
{
	if (!bEnableControllerVibration) return;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const float FinalIntensity = FMath::Clamp(Intensity * VibrationIntensity, 0.0f, 1.0f);
		PC->PlayDynamicForceFeedback(
			FinalIntensity,
			Duration,
			bHeavyLeft,
			bLightRight,
			bHeavyLeft,
			bLightRight,
			EDynamicForceFeedbackAction::Start
		);
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
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &AGAME3023_F26_RoshanCharacter::DoInteractRelease);
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Canceled, this, &AGAME3023_F26_RoshanCharacter::DoInteractRelease);
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
	PlayerInputComponent->BindKey(EKeys::E, IE_Released, this, &AGAME3023_F26_RoshanCharacter::DoInteractRelease);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);

	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Released, this, &AGAME3023_F26_RoshanCharacter::DoInteractRelease);

	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
}

// Move: receives the player's movement input and sends it to the character.
void AGAME3023_F26_RoshanCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	bMovedViaEnhancedInputThisFrame = true;
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

	if (!Cylinder->bHasBattery && HasBattery())
	{
		bIsHoldingToAttach = true;
		HoldingTargetCylinder = Cylinder;
		CurrentAttachHoldTime = 0.0f;
		HoldRumblePulseTimer = 0.0f;
		TriggerControllerRumble(0.25f, 0.08f, false, true);
		return true;
	}

	ResetBatteryAttachHold();
	if (Cylinder->bHasBattery)
	{
		bRequireFreshPressForHold = true;
	}
	Cylinder->Interact(this);
	return true;
}

// DoInteract: starts an pickup or cylinder interaction when the interact key is pressed.
void AGAME3023_F26_RoshanCharacter::DoInteract()
{
	if (bIsDead || LastInteractPressFrame == GFrameCounter)
	{
		return;
	}
	LastInteractPressFrame = GFrameCounter;

	bEnhancedInteractHeld = true;
	bRawInteractKeyHeld = true;
	bRequireFreshPressForHold = false;

	if (CurrentInteractable)
	{
		if (ARedPowerCylinder* RedCell = Cast<ARedPowerCylinder>(CurrentInteractable))
		{
			InteractWithCylinder(RedCell);
			return;
		}
		if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(CurrentInteractable))
		{
			ResetBatteryAttachHold();
			bRequireFreshPressForHold = true;
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
					ResetBatteryAttachHold();
					bRequireFreshPressForHold = true;
					Battery->Interact(this);
					return;
				}
			}
		}
	}
}

// DoInteractRelease: stops the held interaction and resets its progress.
void AGAME3023_F26_RoshanCharacter::DoInteractRelease()
{
	bEnhancedInteractHeld = false;
	bRawInteractKeyHeld = false;
	bRequireFreshPressForHold = false;
	ResetBatteryAttachHold();
}

// Handles IsInteractInputHeld interaction logic
bool AGAME3023_F26_RoshanCharacter::IsInteractInputHeld() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const bool bDirectKeyDown =
			PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Left) ||
			PC->IsInputKeyDown(EKeys::E);

		if (bDirectKeyDown)
		{
			return true;
		}
	}

	return bEnhancedInteractHeld || bRawInteractKeyHeld;
}

// GetBatteryAttachHoldProgress: manages battery attachment and state
float AGAME3023_F26_RoshanCharacter::GetBatteryAttachHoldProgress() const
{
	if (!bIsHoldingToAttach || BatteryAttachHoldDuration <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentAttachHoldTime / BatteryAttachHoldDuration, 0.0f, 1.0f);
}

// Clears hold interaction timer and hides progress ring
void AGAME3023_F26_RoshanCharacter::ResetBatteryAttachHold()
{
	bIsHoldingToAttach = false;
	CurrentAttachHoldTime = 0.0f;
	HoldingTargetCylinder = nullptr;
	HoldRumblePulseTimer = 0.0f;

	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetHoldProgress(0.0f);
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
		TriggerControllerRumble(0.8f, 0.15f, true, false);

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

	TriggerControllerRumble(0.5f, 0.18f, false, true);

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

	ResetBatteryAttachHold();

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

// UpdateCylinderHoldPrompt: updates UI display and widget visibility
void AGAME3023_F26_RoshanCharacter::UpdateCylinderHoldPrompt(ARedPowerCylinder* Cylinder, float DeltaTime, bool bInputHeld, FString& OutAction, FString& OutTarget)
{
	if (Cylinder->bHasBattery)
	{
		ResetBatteryAttachHold();
		InteractionPromptWidget->SetHoldProgress(0.0f);
		OutAction = TEXT("Press Square to Collect Battery");
		OutTarget = FString::Printf(TEXT("Laser Gun (ID: %d) - Online"), Cylinder->CellID);
		return;
	}

	if (!HasBattery())
	{
		ResetBatteryAttachHold();
		InteractionPromptWidget->SetHoldProgress(0.0f);
		OutAction = TEXT("Laser Gun (Requires Battery)");
		OutTarget = FString::Printf(TEXT("Laser Gun (ID: %d) - Offline"), Cylinder->CellID);
		return;
	}

	if (!bInputHeld || bRequireFreshPressForHold || bIsDead)
	{
		ResetBatteryAttachHold();
		InteractionPromptWidget->SetHoldProgress(0.0f);
		OutAction = FString::Printf(TEXT("Hold Square (%.0fs) to Attach Battery"), FMath::Max(BatteryAttachHoldDuration, 0.5f));
		OutTarget = FString::Printf(TEXT("Laser Gun (ID: %d) - Ready"), Cylinder->CellID);
		return;
	}

	if (!bIsHoldingToAttach || HoldingTargetCylinder != Cylinder)
	{
		bIsHoldingToAttach = true;
		HoldingTargetCylinder = Cylinder;
		CurrentAttachHoldTime = 0.0f;
		HoldRumblePulseTimer = 0.0f;
	}

	CurrentAttachHoldTime += DeltaTime;
	HoldRumblePulseTimer += DeltaTime;

	const float SafeHoldDuration = FMath::Max(BatteryAttachHoldDuration, 0.5f);
	const float Progress = FMath::Clamp(CurrentAttachHoldTime / SafeHoldDuration, 0.0f, 1.0f);
	InteractionPromptWidget->SetHoldProgress(Progress);

	if (HoldRumblePulseTimer >= 0.25f)
	{
		HoldRumblePulseTimer = 0.0f;
		TriggerControllerRumble(0.15f + 0.35f * Progress, 0.06f, false, true);
	}

	if (CurrentAttachHoldTime >= SafeHoldDuration)
	{
		ResetBatteryAttachHold();
		bRequireFreshPressForHold = true;
		TriggerControllerRumble(0.85f, 0.25f, true, true);
		Cylinder->Interact(this);

		OutAction = TEXT("Press Square to Collect Battery");
		OutTarget = FString::Printf(TEXT("Laser Gun (ID: %d) - Online"), Cylinder->CellID);
		return;
	}

	OutAction = FString::Printf(TEXT("Attaching Battery... %.1fs / %.0fs"), CurrentAttachHoldTime, SafeHoldDuration);
	OutTarget = FString::Printf(TEXT("Laser Gun (ID: %d) - Charging"), Cylinder->CellID);
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

	const bool bInputCurrentlyHeld = IsInteractInputHeld();
	if (!bInputCurrentlyHeld)
	{
		bRequireFreshPressForHold = false;
		ResetBatteryAttachHold();
	}

	AActor* BestCandidate = FindBestInteractable();
	if (!BestCandidate)
	{
		ResetBatteryAttachHold();
		CurrentInteractable = nullptr;
		InteractionPromptWidget->HidePrompt();
		return;
	}

	CurrentInteractable = BestCandidate;
	InteractionPromptWidget->bAnchorToScreenBottom = bAnchorPromptToScreenBottom;

	FString ActionStr = TEXT("Press Square to Interact");
	FString TargetStr;

	if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(BestCandidate))
	{
		ResetBatteryAttachHold();
		InteractionPromptWidget->SetHoldProgress(0.0f);
		ActionStr = HasBattery()
			? TEXT("Hands Full (Drop Battery First)")
			: TEXT("Press Square to Collect Battery");
		TargetStr = TEXT("Battery Capsule");
	}
	else if (ARedPowerCylinder* Cylinder = Cast<ARedPowerCylinder>(BestCandidate))
	{
		UpdateCylinderHoldPrompt(Cylinder, DeltaTime, bInputCurrentlyHeld, ActionStr, TargetStr);
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

// SetBGMVolume: handles set bgm volume
void AGAME3023_F26_RoshanCharacter::SetBGMVolume(float NewVolume)
{
	BGMVolume = FMath::Clamp(NewVolume, 0.0f, 2.0f);
	if (BGMAudioComponent)
	{
		BGMAudioComponent->SetVolumeMultiplier(BGMVolume);
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
	ResetBatteryAttachHold();
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

	if (bEnableControllerVibration)
	{
		TriggerControllerRumble(1.0f, 0.5f, true, true);
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

	ResetBatteryAttachHold();

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

	SmoothedOcclusionTargetPos = RespawnLocation + FVector(0.0f, 0.0f, 35.0f);

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
	HealPlayer(Healing);
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

	if (bEnableControllerVibration)
	{
		TriggerControllerRumble(0.75f, 0.2f, true, false);
	}

	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
}

// HealPlayer: handles heal player
void AGAME3023_F26_RoshanCharacter::HealPlayer(float HealAmount)
{
	if (bIsDead || HealAmount <= 0.0f || CurrentHealth >= MaxHealth)
	{
		return;
	}

	ApplyHealthChange(CurrentHealth + HealAmount);
	UE_LOG(LogTemplateCharacter, Log, TEXT("[LiveWire Health] Player healed for %.1f. Health: %.1f / %.1f"),
		HealAmount, CurrentHealth, MaxHealth);
}

// SetHealth: updates health and processes damage
void AGAME3023_F26_RoshanCharacter::SetHealth(float NewHealth)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	if (bIsDead && Clamped > 0.0f)
	{
		bIsDead = false;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	ApplyHealthChange(Clamped);

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		HandleDeath();
	}
}

// ResetHealth: updates health and processes damage
void AGAME3023_F26_RoshanCharacter::ResetHealth()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimerHandle);
	}
	bIsDead = false;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	SetHealth(MaxHealth);
}

