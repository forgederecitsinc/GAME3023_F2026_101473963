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

	// 2.5d imax top-down camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// locking absolute rotation so the boom NEVER rotates when player turns
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = CameraDistance;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	CameraBoom->bUsePawnControlRotation = false; // do not spin with mouse
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	// turning off collision test so the camera doesn't zoom in crazy when walking under catwalks
	CameraBoom->bDoCollisionTest = bEnableCameraCollision;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 100.0f);

	// camera lag gives that smooth drone/crane feel
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->CameraLagMaxDistance = 600.0f;

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

	// reset controller rotation to forward
	if (AController* PC = GetController())
	{
		PC->SetControlRotation(FRotator(0.0f, CameraYaw, 0.0f));
	}
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

	// reset flag at end of frame
	bMovedViaEnhancedInputThisFrame = false;
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
		CameraBoom->SetWorldRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
		CameraBoom->TargetArmLength = CameraDistance;
		CameraBoom->bUsePawnControlRotation = false;
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw = false;
		CameraBoom->bInheritRoll = false;
		CameraBoom->bDoCollisionTest = bEnableCameraCollision;
		CameraBoom->ProbeSize = 12.0f;
		CameraBoom->bEnableCameraLag = true;
		CameraBoom->CameraLagSpeed = CameraLagSpeed;
		CameraBoom->CameraLagMaxDistance = 600.0f;
		CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 100.0f);
	}

	if (FollowCamera)
	{
		FollowCamera->SetFieldOfView(CameraFOV);
		FollowCamera->bUsePawnControlRotation = false;
		FollowCamera->SetRelativeRotation(FRotator::ZeroRotator);
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
	// do nothing - camera is fixed
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AGAME3023_F26_RoshanCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// 2.5d top-down movement:
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
	// intentionally left empty - top down camera doesn't rotate with mouse
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
