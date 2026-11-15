// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: RedPowerCylinder.cpp
// Description: Implementation of the Laser Gun Turret (Red Power Cylinder).
// Handles battery cradle socketing, laser raycasting, target detection,
// and line-of-sight elevation checks so players can't interact through walls.
// =============================================================================

#include "RedPowerCylinder.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "BatteryCapsule.h"
#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "GAME3023_F26_Roshan.h"
#include "Kismet/KismetSystemLibrary.h"

ARedPowerCylinder::ARedPowerCylinder()
{
	PrimaryActorTick.bCanEverTick = true;

	// root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// socket where the battery snaps in when inserted
	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(-20.0f, 0.0f, 360.0f));

	// laser emitter socket
	LaserEmitter = CreateDefaultSubobject<USceneComponent>(TEXT("LaserEmitter"));
	LaserEmitter->SetupAttachment(RootComponent);
	LaserEmitter->SetRelativeLocation(FVector(322.0f, 0.0f, 286.0f));
	LaserEmitter->SetRelativeRotation(FRotator::ZeroRotator);

	// visual laser beam mesh
	LaserBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeamMesh"));
	LaserBeamMesh->SetupAttachment(LaserEmitter);
	LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamMesh->SetCastShadow(false);
	LaserBeamMesh->SetVisibility(false); // off until battery is attached

	// interaction trigger zone for player E / controller square
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	InteractionZone->SetSphereRadius(240.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::BuildGunComponents()
{
	const FRotator AimRot(0.0f, GunYaw, 0.0f);

	// configure battery socket
	if (BatterySocket)
	{
		const FVector SocketLoc = AimRot.RotateVector(FVector(-20.0f, 0.0f, 75.0f)) + FVector(0.0f, 0.0f, 286.0f);
		BatterySocket->SetRelativeLocation(SocketLoc);
		BatterySocket->SetRelativeRotation(AimRot);
	}

	// configure laser emitter
	if (LaserEmitter)
	{
		const FVector EmitterLoc = AimRot.RotateVector(FVector(322.0f, 0.0f, 0.0f)) + FVector(0.0f, 0.0f, 286.0f);
		LaserEmitter->SetRelativeLocation(EmitterLoc);
		LaserEmitter->SetRelativeRotation(AimRot);
	}

	// configure laser beam mesh
	if (LaserBeamMesh)
	{
		LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LaserBeamMesh->SetCastShadow(false);
	}

	// configure interaction trigger zone
	if (InteractionZone)
	{
		InteractionZone->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
		InteractionZone->SetSphereRadius(240.0f);
	}
}

void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildGunComponents();
	ApplyColorFromID();
	UpdateLaserBeam();
}

#if WITH_EDITOR
void ARedPowerCylinder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyColorFromID();
	UpdateLaserBeam();
}
#endif

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	BuildGunComponents();
	ApplyColorFromID();
	UpdateLaserBeam();
}

FLinearColor ARedPowerCylinder::GetColorForID(int32 InID) const
{
	// vibrant saturated color palette matching the blockout art in the arena
	switch (InID)
	{
		case 1001: return FLinearColor(1.00f, 0.10f, 0.12f); // vibrant cherry red
		case 1002: return FLinearColor(0.08f, 0.95f, 0.18f); // vibrant electric emerald green
		case 1003: return FLinearColor(0.08f, 0.45f, 1.00f); // vibrant cobalt electric blue
		case 1004: return FLinearColor(1.00f, 0.82f, 0.02f); // vibrant hazard caution yellow (matches arena turrets)
		case 1005: return FLinearColor(1.00f, 0.42f, 0.02f); // vibrant safety neon orange
		case 1006: return FLinearColor(0.78f, 0.10f, 1.00f); // vibrant neon electric violet
		case 1007: return FLinearColor(0.02f, 0.92f, 0.98f); // vibrant bright laser cyan
		case 1008: return FLinearColor(1.00f, 0.10f, 0.65f); // vibrant hot neon magenta
		default:
		{
			// high saturation and high brightness for any custom 4-digit ID
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 240, 255);
		}
	}
}

void ARedPowerCylinder::ApplyColorFromID()
{
	// We do NOT color or override static meshes in C++!
	// Any materials assigned by the student in the Blueprint (BP_RedPowerCylinder) or on the mesh components
	// are preserved exactly as they are without being overwritten or tinted by C++.
}

bool ARedPowerCylinder::CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const
{
	if (!ActorToCheck) return false;

	// 1. Direct LaserTarget check
	if (ALaserTarget* LaserTarget = Cast<ALaserTarget>(ActorToCheck))
	{
		OutFoundID = LaserTarget->TargetID;
		return (LaserTarget->TargetID == CellID);
	}

	// 2. Direct TransformerBushing check
	if (ATransformerBushing* Bushing = Cast<ATransformerBushing>(ActorToCheck))
	{
		OutFoundID = Bushing->BushingID;
		return (Bushing->BushingID == CellID);
	}

	// 2. Direct other RedPowerCylinder check
	if (ARedPowerCylinder* OtherCyl = Cast<ARedPowerCylinder>(ActorToCheck))
	{
		if (OtherCyl != this)
		{
			OutFoundID = OtherCyl->CellID;
			return (OtherCyl->CellID == CellID);
		}
	}

	// 3. Check attached parent actor (in case a child mesh component was hit)
	if (AActor* ParentActor = ActorToCheck->GetAttachParentActor())
	{
		if (ParentActor != ActorToCheck)
		{
			if (CheckActorTargetIDMatch(ParentActor, OutFoundID))
			{
				return true;
			}
		}
	}

	// 4. Generic reflection: check for integer property named "TargetID" or "CellID"
	if (FIntProperty* TargetIdProp = CastField<FIntProperty>(ActorToCheck->GetClass()->FindPropertyByName(TEXT("TargetID"))))
	{
		OutFoundID = TargetIdProp->GetPropertyValue_InContainer(ActorToCheck);
		return (OutFoundID == CellID);
	}
	if (FIntProperty* CellIdProp = CastField<FIntProperty>(ActorToCheck->GetClass()->FindPropertyByName(TEXT("CellID"))))
	{
		OutFoundID = CellIdProp->GetPropertyValue_InContainer(ActorToCheck);
		return (OutFoundID == CellID);
	}

	return false;
}

void ARedPowerCylinder::RotateGun(float AngleDegrees)
{
	GunYaw = FMath::Fmod(GunYaw + AngleDegrees, 360.0f);
	BuildGunComponents();
	UpdateLaserBeam();
}

void ARedPowerCylinder::UpdateLaserBeam()
{
	if (!LaserBeamMesh || !LaserEmitter) return;

	// laser only turns on once battery is attached and powers on the gun!
	// (or when previewing in editor details panel)
	const bool bShouldBeActive = (bIsPowered && bHasBattery) || (bPreviewLaserInEditor && GetWorld() && !GetWorld()->IsGameWorld());

	if (!bShouldBeActive)
	{
		LaserBeamMesh->SetVisibility(false);
		bIsTargetDetected = false;
		DetectedTarget = nullptr;
		bHasPrintedNoTarget = false;
		bHasPrintedTargetDetected = false;
		if (LastHitLaserTarget)
		{
			LastHitLaserTarget->SetLaserHit(false, this);
			LastHitLaserTarget = nullptr;
		}
		if (LastHitBushing)
		{
			LastHitBushing->SetLaserHit(false, this);
			LastHitBushing = nullptr;
		}
		return;
	}

	LaserBeamMesh->SetVisibility(true);

	const FVector Start = LaserEmitter->GetComponentLocation();
	const FVector Forward = LaserEmitter->GetForwardVector();
	const FVector DesiredEnd = Start + Forward * LaserLength;

	float ActualDistance = LaserLength;
	AActor* HitActor = nullptr;

	// raycast forward so the laser beam stops against obstacles and detects target
	if (GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (InsertedBattery)
		{
			Params.AddIgnoredActor(InsertedBattery);
		}

		// Line trace against Visibility channel (hits static geometry, props, and dynamic actors)
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, DesiredEnd, ECC_Visibility, Params);
		if (bHit)
		{
			HitActor = Hit.GetActor();
			if (bLaserStopsAtObstacles)
			{
				ActualDistance = FMath::Max(15.0f, Hit.Distance);
			}
		}
	}

	// position laser beam: cube mesh center is at 0, so offset forward by half distance along X
	// scale X by distance, scale Y and Z by laser thickness
	LaserBeamMesh->SetRelativeLocation(FVector(ActualDistance * 0.5f, 0.0f, 0.0f));
	LaserBeamMesh->SetRelativeRotation(FRotator::ZeroRotator);
	LaserBeamMesh->SetRelativeScale3D(FVector(ActualDistance / 100.0f, LaserThickness / 100.0f, LaserThickness / 100.0f));

	// Target detection by matching Target ID during gameplay
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		// 1. Check if laser beam hits player character and deal laser burn damage
		AGAME3023_F26_RoshanCharacter* HitPlayer = Cast<AGAME3023_F26_RoshanCharacter>(HitActor);
		if (!HitPlayer && HitActor)
		{
			HitPlayer = Cast<AGAME3023_F26_RoshanCharacter>(HitActor->GetAttachParentActor());
		}

		if (HitPlayer && !HitPlayer->IsDead())
		{
			const float DeltaSec = GetWorld()->GetDeltaSeconds();
			PlayerLaserDamageTimer += DeltaSec;
			if (PlayerLaserDamageTimer >= 0.35f)
			{
				PlayerLaserDamageTimer = 0.0f;
				HitPlayer->ApplyDamageToPlayer(10.0f, this);
			}
		}
		else
		{
			PlayerLaserDamageTimer = 0.35f;
		}

		int32 FoundTargetID = -1;
		const bool bHitMatchingTarget = CheckActorTargetIDMatch(HitActor, FoundTargetID);
		bIsTargetDetected = bHitMatchingTarget;
		DetectedTarget = bHitMatchingTarget ? HitActor : nullptr;

		// Manage LaserTarget and TransformerBushing glow transitions
		ALaserTarget* CurrentHitLaserTarget = Cast<ALaserTarget>(HitActor);
		ATransformerBushing* CurrentHitBushing = Cast<ATransformerBushing>(HitActor);
		if (HitActor)
		{
			if (!CurrentHitLaserTarget) CurrentHitLaserTarget = Cast<ALaserTarget>(HitActor->GetAttachParentActor());
			if (!CurrentHitBushing) CurrentHitBushing = Cast<ATransformerBushing>(HitActor->GetAttachParentActor());
		}

		if (LastHitLaserTarget && LastHitLaserTarget != CurrentHitLaserTarget)
		{
			LastHitLaserTarget->SetLaserHit(false, this);
			LastHitLaserTarget = nullptr;
		}

		if (LastHitBushing && LastHitBushing != CurrentHitBushing)
		{
			LastHitBushing->SetLaserHit(false, this);
			LastHitBushing = nullptr;
		}

		if (bHitMatchingTarget)
		{
			if (CurrentHitLaserTarget)
			{
				CurrentHitLaserTarget->SetLaserHit(true, this);
				LastHitLaserTarget = CurrentHitLaserTarget;
			}
			if (CurrentHitBushing)
			{
				CurrentHitBushing->SetLaserHit(true, this);
				LastHitBushing = CurrentHitBushing;
			}

			// Reset the "no target" latch so it can print again if we lose target
			bHasPrintedNoTarget = false;

			// Print once upon detecting the matching target object (displayed for 3 seconds)
			if (!bHasPrintedTargetDetected)
			{
				bHasPrintedTargetDetected = true;

#if WITH_EDITOR
				const FString TargetName = HitActor ? HitActor->GetActorLabel() : TEXT("Target");
#else
				const FString TargetName = HitActor ? HitActor->GetName() : TEXT("Target");
#endif
				const FString Msg = FString::Printf(TEXT("[TARGET MATCH: %d] Laser hit matching target object: %s"), CellID, *TargetName);

				UKismetSystemLibrary::PrintString(this, Msg, true, true, FLinearColor(0.1f, 1.0f, 0.2f), 3.0f);

				UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Detected matching target '%s' with ID %d (printed for 3s)."), *GetName(), CellID, *TargetName, FoundTargetID);
			}
		}
		else
		{
			// Reset the "target detected" latch so it can print again when matching target is acquired
			bHasPrintedTargetDetected = false;

			// Print once when emitting laser but not detecting a matching target
			if (!bHasPrintedNoTarget)
			{
				bHasPrintedNoTarget = true;

				const FString Msg = FString::Printf(TEXT("[LASER EMITTING] Laser beam emitted (ID: %d) - No matching target detected"), CellID);

				UKismetSystemLibrary::PrintString(this, Msg, true, true, FLinearColor(1.0f, 0.85f, 0.1f), 2.5f);

				UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Laser emitted - no matching target detected (printed once)."), *GetName(), CellID);
			}
		}
	}
}

void ARedPowerCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if battery is attached and powered, continuously update laser so it reacts if dynamic actors cross the beam
	if (bIsPowered && bHasBattery)
	{
		UpdateLaserBeam();
	}
}

bool ARedPowerCylinder::CanPlayerInteract(const AActor* InPlayerActor) const
{
	if (!InPlayerActor)
	{
		return false;
	}

	const FVector PlayerLoc = InPlayerActor->GetActorLocation();
	const FVector CylinderLoc = GetActorLocation();

	// 1. Elevation check:
	// In Unreal, player capsule center is ~90-100 units above player feet.
	// Cylinder base origin (Z=0 relative) sits on top of the platform/ledge.
	// If player is on the platform, PlayerLoc.Z >= CylinderLoc.Z + 50.0f.
	// If player is on the lower floor/path beneath the platform, PlayerLoc.Z will be below (CylinderLoc.Z + 20.0f).
	if (PlayerLoc.Z < (CylinderLoc.Z + 20.0f))
	{
		return false;
	}

	// Player shouldn't be way above the cylinder either
	if (PlayerLoc.Z > (CylinderLoc.Z + 400.0f))
	{
		return false;
	}

	// 2. 2D Horizontal distance check: player must be next to the cylinder
	const float Dist2DSq = FVector::DistSquared2D(PlayerLoc, CylinderLoc);
	if (Dist2DSq > FMath::Square(280.0f))
	{
		return false;
	}

	// 3. Line of sight check:
	// Verify that the platform floor or walls do not block the path between the player and cylinder gun
	UWorld* World = GetWorld();
	if (World)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(InPlayerActor);

		const FVector EyeLoc = PlayerLoc + FVector(0.0f, 0.0f, 40.0f);
		const FVector CylinderTargetLoc = CylinderLoc + FVector(0.0f, 0.0f, 286.0f);

		if (World->LineTraceSingleByChannel(HitResult, EyeLoc, CylinderTargetLoc, ECC_Visibility, QueryParams))
		{
			if (HitResult.bBlockingHit && HitResult.GetActor() && HitResult.GetActor() != this && HitResult.GetActor() != InPlayerActor)
			{
				return false;
			}
		}
	}

	return true;
}

void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (!CanPlayerInteract(Player))
		{
			return;
		}

		Player->SetCurrentInteractable(this);

		if (GEngine)
		{
			if (bHasBattery)
			{
				GEngine->AddOnScreenDebugMessage(
					2001,
					2.5f,
					FColor::Cyan,
					FString::Printf(TEXT("[Laser Gun] Press [Square] to Remove Battery (ID: %d)"), CellID)
				);
			}
			else
			{
				if (Player->HasBattery())
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.5f,
						FColor::Green,
						FString::Printf(TEXT("[Laser Gun] Press [Square] to Attach Battery into Laser Gun (ID: %d)"), CellID)
					);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.5f,
						FColor::Orange,
						FString::Printf(TEXT("[Laser Gun] Laser Gun (ID: %d): OFFLINE (Requires Matching Battery)"), CellID)
					);
				}
			}
		}
	}
}

void ARedPowerCylinder::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (Player->GetCurrentInteractable() == this)
		{
			Player->SetCurrentInteractable(nullptr);
		}
	}
}

bool ARedPowerCylinder::Interact(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player || !CanPlayerInteract(Player)) return false;

	// 1. remove battery if already plugged in
	if (bHasBattery)
	{
		if (Player->HasBattery())
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.5f,
					FColor::Red,
					TEXT("[INVENTORY FULL] Hands full! Press [R] / [Circle] to drop your current battery first."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}
			return false;
		}

		ABatteryCapsule* Extracted = RemoveBattery();
		if (Extracted)
		{
			Player->TryPickupBattery(Extracted);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					4.0f,
					FColor::Yellow,
					FString::Printf(TEXT("[BATTERY REMOVED] Battery removed! Laser Gun (ID: %d) is now OFFLINE."), CellID),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}

			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Battery removed. Laser OFF."), *GetName(), CellID);
			return true;
		}
		return false;
	}

	// 2. insert battery into empty laser gun
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		const bool bMatched = InsertBattery(BatteryToSocket);

		if (bMatched)
		{
			Player->TriggerControllerRumble(1.0f, 0.35f, true, true);
		}
		else
		{
			Player->TriggerControllerRumble(0.6f, 0.15f, true, false);
		}

		return bMatched;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.5f,
				FColor::Red,
				FString::Printf(TEXT("[POWER OFFLINE] Laser Gun (ID: %d) needs a matching Battery!"), CellID),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] Laser Gun %s (ID: %d): Interacted without a battery."), *GetName(), CellID);
		return false;
	}
}

ABatteryCapsule* ARedPowerCylinder::RemoveBattery()
{
	if (!bHasBattery || !InsertedBattery) return nullptr;

	ABatteryCapsule* Extracted = InsertedBattery;

	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;
	bIsTargetDetected = false;
	DetectedTarget = nullptr;
	bHasPrintedNoTarget = false;
	bHasPrintedTargetDetected = false;

	if (LastHitLaserTarget)
	{
		LastHitLaserTarget->SetLaserHit(false, this);
		LastHitLaserTarget = nullptr;
	}
	if (LastHitBushing)
	{
		LastHitBushing->SetLaserHit(false, this);
		LastHitBushing = nullptr;
	}

	UpdateVisualState();
	UpdateLaserBeam(); // turns off laser!

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;

	// attach battery to socket on top of the laser gun
	Battery->OnSocketed(this, BatterySocket);

	// 4-digit ID match check: battery id must equal cell id to power on!
	if (Battery->BatteryID == CellID)
	{
		bIsPowered = true;
		bHasPrintedNoTarget = false;
		bHasPrintedTargetDetected = false;
		bIsTargetDetected = false;
		UpdateVisualState(); // glows gun receiver and barrel
		UpdateLaserBeam();   // fires the laser beam out of the nozzle!

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.5f,
				FColor::Green,
				FString::Printf(TEXT("[ID MATCH: %d] Battery attached & Laser Gun ONLINE! Laser beam active!"), CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s: ID MATCH (%d == %d). Laser fired!"), *GetName(), CellID, Battery->BatteryID);
		return true;
	}
	else
	{
		// wrong ID: attaches, but stays unpowered and laser stays off
		bIsPowered = false;
		UpdateVisualState();
		UpdateLaserBeam();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Orange,
				FString::Printf(TEXT("[ID MISMATCH] Battery (ID: %d) attached, but Laser Gun (ID: %d) WON'T POWER ON!"), Battery->BatteryID, CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] Laser Gun %s: ID MISMATCH (Battery ID: %d != Cell ID: %d). Attached but laser OFF!"), *GetName(), Battery->BatteryID, CellID);
		return false;
	}
}

void ARedPowerCylinder::UpdateVisualState()
{
	// Materials are set in Blueprint, not overwritten in C++
	ApplyColorFromID();
}
