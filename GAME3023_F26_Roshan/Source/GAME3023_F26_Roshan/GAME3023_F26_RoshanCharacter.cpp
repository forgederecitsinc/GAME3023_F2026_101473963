// roshan - 101473963 - game3023 game engines 3
// character cpp implementation
// includes 2.5d imax top-down camera setup, wasd movement, and battery inventory management

#include "GAME3023_F26_RoshanCharacter.h"
#include "Engine/LocalPlayer.h"
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

AGAME3023_F26_RoshanCharacter::AGAME3023_F26_RoshanCharacter()
{
	// enable tick so we can poll dualsense analog sticks and twin-stick aiming smoothly
	PrimaryActorTick.bCanEverTick = true;

	// player capsule size
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// dont rotate pawn with controller rotation
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// make character turn towards where they are walking
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 650.0f, 0.0f); // snappy rotation speed

	// movement params from template
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// hades style 2.5d top-down cinematic camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// locking absolute rotation so the boom NEVER rotates when player turns
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	CameraBoom->bUsePawnControlRotation = false; // do not spin with mouse or controller
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	// turning off collision test so the camera doesn't zoom in crazy when walking under catwalks
	CameraBoom->bDoCollisionTest = bEnableCameraCollision;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->SocketOffset = CameraSocketOffset;

	// camera lag gives that smooth drone crane feel
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->CameraLagMaxDistance = 600.0f;
	CameraBoom->bEnableCameraRotationLag = false;

	// follow camera with wide imax fov
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(CameraFOV);
}

void AGAME3023_F26_RoshanCharacter::BeginPlay()
{
	Super::BeginPlay();

	// apply our camera settings again here so blueprint defaults don't overwrite them
	ApplyCinematicCameraSettings();

	// initialize smoothed cutout target position to player chest
	SmoothedOcclusionTargetPos = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);

	// enable custom depth on player mesh for silhouette rendering if needed
	if (GetMesh())
	{
		GetMesh()->SetRenderCustomDepth(true);
		GetMesh()->SetCustomDepthStencilValue(1);
	}

	// reset controller rotation to forward
	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}
}

void AGAME3023_F26_RoshanCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AController* PC = NewController)
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}
}

void AGAME3023_F26_RoshanCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// put back original materials on any roof meshes before ending play
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

	Super::EndPlay(EndPlayReason);
}

void AGAME3023_F26_RoshanCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// handle dualsense analog sticks and d-pad directly in c++ so it always works even without custom IMC assets
	if (bEnableDualSenseController)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			// 1. left analog stick + d-pad movement fallback (if enhanced input didn't already move us this frame)
			if (!bMovedViaEnhancedInputThisFrame)
			{
				float StickX = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
				float StickY = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY);

				// also let the player walk around using the dualsense d-pad
				if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Right)) StickX += 1.0f;
				if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Left))  StickX -= 1.0f;
				if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Up))    StickY += 1.0f;
				if (PC->IsInputKeyDown(EKeys::Gamepad_DPad_Down))  StickY -= 1.0f;

				FVector2D StickVec(StickX, StickY);
				if (StickVec.Size() > ControllerStickDeadZone)
				{
					// clamp to 1.0 so diagonal d-pad or stick doesn't make us run faster
					if (StickVec.SizeSquared() > 1.0f)
					{
						StickVec.Normalize();
					}
					DoMove(StickVec.X, StickVec.Y);
				}
			}

			// 2. right analog stick twin-stick aiming so player can aim their character in 360 degrees
			if (bEnableRightStickAiming)
			{
				const float RightX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
				const float RightY = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY);
				const FVector2D AimVec(RightX, RightY);

				if (AimVec.Size() > ControllerStickDeadZone)
				{
					// convert right stick direction into world space relative to our fixed camera yaw
					const FRotator CamYawRot(0.0f, CameraYaw, 0.0f);
					const FVector AimWorldDir =
						FRotationMatrix(CamYawRot).GetUnitAxis(EAxis::X) * AimVec.Y +
						FRotationMatrix(CamYawRot).GetUnitAxis(EAxis::Y) * AimVec.X;

					if (!AimWorldDir.IsNearlyZero())
					{
						// temporarily pause orient-to-movement while holding right stick so character faces aim direction
						if (GetCharacterMovement())
						{
							GetCharacterMovement()->bOrientRotationToMovement = false;
						}

						const FRotator TargetRot = AimWorldDir.Rotation();
						const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, TargetRot.Yaw, 0.0f), DeltaTime, 15.0f);
						SetActorRotation(NewRot);
					}
				}
				else
				{
					// when right stick is let go, go back to facing where we walk
					if (GetCharacterMovement() && !GetCharacterMovement()->bOrientRotationToMovement)
					{
						GetCharacterMovement()->bOrientRotationToMovement = true;
					}
				}
			}
		}
	}

	// update smooth circular see-through cutout on any roofs or catwalks hiding the player
	UpdateCameraOcclusionCutout(DeltaTime);

	// reset flag at end of frame
	bMovedViaEnhancedInputThisFrame = false;
}

void AGAME3023_F26_RoshanCharacter::UpdateCameraOcclusionCutout(float DeltaTime)
{
	if (!bEnableOcclusionCutout || !FollowCamera || !GetWorld())
	{
		return;
	}

	const FVector CameraLoc = FollowCamera->GetComponentLocation();
	const FVector PlayerTargetLoc = GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);

	// smoothly glide the cutout lens center so it feels organic when moving fast
	SmoothedOcclusionTargetPos = FMath::VInterpTo(SmoothedOcclusionTargetPos, PlayerTargetLoc, DeltaTime, 18.0f);

	// mark all currently tracked meshes as not hit yet for this frame
	for (auto& Pair : ActiveOccludingMeshes)
	{
		Pair.Value.bHitThisFrame = false;
	}

	// sweep a sphere along the sightline from the camera to the player's torso
	// using SweepMultiByObjectType so if there are multiple overlapping roof pieces, we catch ALL of them
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

	const float PlayerFeetZ = GetActorLocation().Z - 20.0f;

	for (const FHitResult& Hit : Hits)
	{
		UPrimitiveComponent* HitComp = Hit.GetComponent();
		AActor* HitActor = Hit.GetActor();
		if (!HitComp || !HitActor || HitActor == this || HitActor == CarriedBattery)
		{
			continue;
		}

		// ignore gameplay pickups/cylinders so we don't accidentally mask batteries or power cells
		if (HitActor->IsA(ABatteryCapsule::StaticClass()) || HitActor->IsA(ARedPowerCylinder::StaticClass()))
		{
			continue;
		}

		// skip the floor mesh that the player is standing on!
		// only cut out meshes whose top surface is higher than the player's waist
		const FBox CompBox = HitComp->Bounds.GetBox();
		if (CompBox.Max.Z < (PlayerFeetZ + 60.0f))
		{
			continue;
		}

		FOccludingMeshData* ExistingData = ActiveOccludingMeshes.Find(HitComp);
		if (!ExistingData)
		{
			// first frame this roof/catwalk is hiding the player!
			// save its original materials and swap in our circular cutout dynamic material instances
			FOccludingMeshData NewData;
			NewData.CurrentAlpha = 0.0f;
			NewData.bHitThisFrame = true;

			const int32 NumMats = HitComp->GetNumMaterials();
			NewData.OriginalMaterials.Reserve(NumMats);
			NewData.CutoutMIDs.Reserve(NumMats);

				for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
				{
					UMaterialInterface* OrigMat = HitComp->GetMaterial(MatIdx);
					NewData.OriginalMaterials.Add(OrigMat);

					// only swap materials that actually declare the authored cutout feature, so we
					// never touch engine content or anything that would lose its original look
					UMaterialInstanceDynamic* CutoutMID = nullptr;
					float StrengthCheck = 0.0f;
					if (OrigMat && OrigMat->GetScalarParameterValue(FMaterialParameterInfo(TEXT("OcclusionStrength")), StrengthCheck))
					{
						// a dynamic instance OF the original material keeps every colour, grid scale
						// and texture override it already had, so the object keeps its exact look and
						// only the circular patch above the player fades out
						CutoutMID = UMaterialInstanceDynamic::Create(OrigMat, this);
						if (CutoutMID)
						{
							HitComp->SetMaterial(MatIdx, CutoutMID);
						}
					}
					NewData.CutoutMIDs.Add(CutoutMID);
				}

			ActiveOccludingMeshes.Add(HitComp, MoveTemp(NewData));
		}
		else
		{
			ExistingData->bHitThisFrame = true;
		}
	}

	// now smoothly interpolate the cutout alpha (0 -> 1 when hidden, 1 -> 0 when visible again)
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

		// drive the authored cutout feature on each material instance
		const float SafeRadius = FMath::Max(OcclusionCutoutRadius, 20.0f);
		const float SafeSoftness = FMath::Clamp(OcclusionEdgeSoftness, 10.0f, SafeRadius * 0.9f);

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

		// once the cutout has smoothly faded all the way back to solid, restore the original material!
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

void AGAME3023_F26_RoshanCharacter::TriggerControllerRumble(float Intensity, float Duration, bool bHeavyLeft, bool bLightRight)
{
	// play haptic rumble on dualsense controller if enabled
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

void AGAME3023_F26_RoshanCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// applies camera distance/pitch changes in editor viewport
	ApplyCinematicCameraSettings();
}

#if WITH_EDITOR
void AGAME3023_F26_RoshanCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// if user changed TargetArmLength directly on CameraBoom in the components inspector, sync CameraDistance!
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

	// if user changed CameraDistance under Cinematic Camera in inspector, push it straight into CameraBoom
	if (PropName == GET_MEMBER_NAME_CHECKED(AGAME3023_F26_RoshanCharacter, CameraDistance))
	{
		if (CameraBoom)
		{
			CameraBoom->TargetArmLength = CameraDistance;
		}
	}

	// live refresh in viewport
	ApplyCinematicCameraSettings();
}
#endif

void AGAME3023_F26_RoshanCharacter::ApplyCinematicCameraSettings()
{
	// lock controller rotation
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

void AGAME3023_F26_RoshanCharacter::AddControllerYawInput(float Val)
{
	// empty on purpose: player cannot spin the camera with mouse or controller
}

void AGAME3023_F26_RoshanCharacter::AddControllerPitchInput(float Val)
{
	// empty on purpose: player cannot pitch the camera
}

void AGAME3023_F26_RoshanCharacter::AddControllerRollInput(float Val)
{
	// empty on purpose: no roll
}

void AGAME3023_F26_RoshanCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// enhanced input bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// wasd move
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Move);

		// look actions bound to empty Look function just in case
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGAME3023_F26_RoshanCharacter::Look);

		// interact (E)
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
		}

		// drop (R)
		if (DropAction)
		{
			EnhancedInputComponent->BindAction(DropAction, ETriggerEvent::Started, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
		}
	}
	else
	{
		UE_LOG(LogGAME3023_F26_Roshan, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
	}

	// hardcoded key fallbacks so E (interact) and R/Q (drop) work without needing to setup IMC assets
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery); // Q also works as drop backup

	// dualsense ps5 controller button bindings so gamepad works out of the box!
	// cross (X button on ps5) -> jump
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoJumpStart);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Released, this, &AGAME3023_F26_RoshanCharacter::DoJumpEnd);

	// square button OR R1 bumper -> interact (pick up battery / plug into cylinder)
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);
	PlayerInputComponent->BindKey(EKeys::Gamepad_RightShoulder, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoInteract);

	// circle button OR L1 bumper -> drop battery onto floor
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
	PlayerInputComponent->BindKey(EKeys::Gamepad_LeftShoulder, IE_Pressed, this, &AGAME3023_F26_RoshanCharacter::DoDropBattery);
}

void AGAME3023_F26_RoshanCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	bMovedViaEnhancedInputThisFrame = true;
	DoMove(MovementVector.X, MovementVector.Y);
}

void AGAME3023_F26_RoshanCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGAME3023_F26_RoshanCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// 2.5d hades top-down movement:
		// W is always screen up (+X)
		// S is always screen down (-X)
		// D is always screen right (+Y)
		// A is always screen left (-Y)
		const FRotator YawRotation(0.0f, CameraYaw, 0.0f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AGAME3023_F26_RoshanCharacter::DoLook(float Yaw, float Pitch)
{
	// intentionally left empty: player cannot spin or pitch the top-down 2.5d camera
}

void AGAME3023_F26_RoshanCharacter::DoJumpStart()
{
	Jump();
}

void AGAME3023_F26_RoshanCharacter::DoJumpEnd()
{
	StopJumping();
}

void AGAME3023_F26_RoshanCharacter::DoInteract()
{
	// step 1: check if we are currently inside an overlap trigger
	if (CurrentInteractable)
	{
		if (ARedPowerCylinder* RedCell = Cast<ARedPowerCylinder>(CurrentInteractable))
		{
			RedCell->Interact(this);
			return;
		}

		if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(CurrentInteractable))
		{
			Battery->Interact(this);
			return;
		}
	}

	// step 2: fallback sphere sweep around player just in case overlap didn't trigger
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector TraceStart = GetActorLocation();
	const FVector TraceEnd = TraceStart + GetActorForwardVector() * 100.0f;
	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(180.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (World->SweepMultiByChannel(HitResults, TraceStart, TraceEnd, FQuat::Identity, ECC_WorldDynamic, Sphere, QueryParams))
	{
		// prioritize cylinder if player is holding a battery
		for (const FHitResult& Hit : HitResults)
		{
			if (ARedPowerCylinder* RedCell = Cast<ARedPowerCylinder>(Hit.GetActor()))
			{
				RedCell->Interact(this);
				return;
			}
		}

		// otherwise grab nearby battery
		for (const FHitResult& Hit : HitResults)
		{
			if (ABatteryCapsule* Battery = Cast<ABatteryCapsule>(Hit.GetActor()))
			{
				if (!Battery->bIsCarried && !Battery->bIsSocketed)
				{
					Battery->Interact(this);
					return;
				}
			}
		}
	}
}

ABatteryCapsule* AGAME3023_F26_RoshanCharacter::TakeCarriedBattery()
{
	if (!HasBattery()) return nullptr;

	ABatteryCapsule* Taken = CarriedBattery;
	CarriedBattery = nullptr;
	return Taken;
}

bool AGAME3023_F26_RoshanCharacter::TryPickupBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	// assignment rule: player can only carry 1 battery at a time!
	// if already holding one, reject it and print debug message
	if (HasBattery())
	{
		// short error buzz on dualsense controller
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

	// crisp pickup vibration on dualsense controller
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

void AGAME3023_F26_RoshanCharacter::DoDropBattery()
{
	if (HasBattery())
	{
		DropCurrentBattery();
	}
	else
	{
		// nothing to drop
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

ABatteryCapsule* AGAME3023_F26_RoshanCharacter::DropCurrentBattery()
{
	if (!HasBattery()) return nullptr;

	ABatteryCapsule* Dropped = CarriedBattery;
	CarriedBattery = nullptr;

	// place battery in front of player
	const FVector ForwardOffset = GetActorForwardVector() * 120.0f;
	FVector DropLocation = GetActorLocation() + ForwardOffset;

	// do a line trace down so it lands on top of floor instead of floating
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
