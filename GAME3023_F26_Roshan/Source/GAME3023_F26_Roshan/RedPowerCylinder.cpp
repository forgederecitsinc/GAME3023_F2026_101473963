#include "RedPowerCylinder.h"
#include "BatteryCapsule.h"
#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "GAME3023_F26_Roshan.h"
#include "LiveWireUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetSystemLibrary.h"

ARedPowerCylinder::ARedPowerCylinder()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(-20.0f, 0.0f, 360.0f));

	LaserEmitter = CreateDefaultSubobject<USceneComponent>(TEXT("LaserEmitter"));
	LaserEmitter->SetupAttachment(RootComponent);
	LaserEmitter->SetRelativeLocation(FVector(322.0f, 0.0f, 286.0f));

	LaserBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeamMesh"));
	LaserBeamMesh->SetupAttachment(LaserEmitter);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LaserCubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (LaserCubeAsset.Succeeded())
	{
		LaserBeamMesh->SetStaticMesh(LaserCubeAsset.Object);
	}
	LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamMesh->SetCastShadow(false);
	LaserBeamMesh->SetVisibility(false);

	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	InteractionZone->SetSphereRadius(240.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::BuildGunComponents()
{
	const FRotator AimRot(0.0f, GunYaw, 0.0f);

	if (BatterySocket)
	{
		BatterySocket->SetRelativeLocation(AimRot.RotateVector(FVector(-20.0f, 0.0f, 75.0f)) + FVector(0.0f, 0.0f, 286.0f));
		BatterySocket->SetRelativeRotation(AimRot);
	}

	if (LaserEmitter)
	{
		LaserEmitter->SetRelativeLocation(AimRot.RotateVector(FVector(322.0f, 0.0f, 0.0f)) + FVector(0.0f, 0.0f, 286.0f));
		LaserEmitter->SetRelativeRotation(AimRot);
	}

	if (LaserBeamMesh)
	{
		LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LaserBeamMesh->SetCastShadow(false);
	}

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
	InitPowerDome();
	ApplyColorFromID();
	UpdateLaserBeam();
}

void ARedPowerCylinder::InitPowerDome()
{
	if (!PowerDomeMesh)
	{
		TArray<UStaticMeshComponent*> StaticMeshes;
		GetComponents<UStaticMeshComponent>(StaticMeshes);
		for (UStaticMeshComponent* MeshComp : StaticMeshes)
		{
			if (MeshComp && MeshComp->GetName().Equals(TEXT("powerDome"), ESearchCase::IgnoreCase))
			{
				PowerDomeMesh = MeshComp;
				break;
			}
		}
	}

	if (!PowerDomeMesh)
	{
		return;
	}

	PowerDomeOriginalMaterials.Empty();
	for (int32 i = 0; i < PowerDomeMesh->GetNumMaterials(); ++i)
	{
		PowerDomeOriginalMaterials.Add(PowerDomeMesh->GetMaterial(i));
	}

	if (!PowerDomeGreenDynMaterial)
	{
		if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			PowerDomeGreenDynMaterial = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (PowerDomeGreenDynMaterial)
			{
				const FLinearColor GreenBase(0.05f, 1.0f, 0.15f, 1.0f);
				const FLinearColor GreenEmissive = GreenBase * PowerDomeGlowIntensity;
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("Color"), GreenBase);
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("BaseColor"), GreenBase);
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("Base Color"), GreenBase);
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), GreenEmissive);
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("Emissive Color"), GreenEmissive);
				PowerDomeGreenDynMaterial->SetVectorParameterValue(TEXT("Emissive"), GreenEmissive);
			}
		}
	}

	if (!PowerDomeLight && PowerDomeLightIntensity > 0.0f && GetWorld() && GetWorld()->IsGameWorld())
	{
		PowerDomeLight = NewObject<UPointLightComponent>(this, TEXT("PowerDomeBlinkLight"));
		if (PowerDomeLight)
		{
			PowerDomeLight->SetupAttachment(PowerDomeMesh);
			PowerDomeLight->SetRelativeLocation(FVector::ZeroVector);
			PowerDomeLight->SetLightColor(FColor(20, 255, 50));
			PowerDomeLight->SetIntensity(PowerDomeLightIntensity);
			PowerDomeLight->SetAttenuationRadius(250.0f);
			PowerDomeLight->SetCastShadows(false);
			PowerDomeLight->SetVisibility(false);
			PowerDomeLight->RegisterComponent();
		}
	}

	bPowerDomeLightOn = (bIsPowered && bHasBattery);
	ApplyPowerDomeLightState(bPowerDomeLightOn);
}

void ARedPowerCylinder::ApplyPowerDomeLightState(bool bLightOn)
{
	const bool bPoweredActive = (bIsPowered && bHasBattery);
	const bool bShowGreen = bPoweredActive && bLightOn;

	if (PowerDomeMesh)
	{
		for (int32 i = 0; i < PowerDomeOriginalMaterials.Num(); ++i)
		{
			if (bShowGreen && PowerDomeGreenDynMaterial)
			{
				PowerDomeMesh->SetMaterial(i, PowerDomeGreenDynMaterial);
			}
			else if (PowerDomeOriginalMaterials[i])
			{
				PowerDomeMesh->SetMaterial(i, PowerDomeOriginalMaterials[i]);
			}
		}
	}

	if (PowerDomeLight)
	{
		PowerDomeLight->SetIntensity(PowerDomeLightIntensity);
		PowerDomeLight->SetLightColor(bShowGreen ? FColor(20, 255, 50) : FColor(255, 130, 15));
		PowerDomeLight->SetVisibility(bPoweredActive);
	}
}

FLinearColor ARedPowerCylinder::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

void ARedPowerCylinder::ApplyColorFromID()
{
}

bool ARedPowerCylinder::CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const
{
	if (!ActorToCheck)
	{
		return false;
	}

	if (ALaserTarget* LaserTarget = Cast<ALaserTarget>(ActorToCheck))
	{
		OutFoundID = LaserTarget->TargetID;
		return OutFoundID == CellID;
	}

	if (ATransformerBushing* Bushing = Cast<ATransformerBushing>(ActorToCheck))
	{
		OutFoundID = Bushing->BushingID;
		return OutFoundID == CellID;
	}

	if (ARedPowerCylinder* OtherCyl = Cast<ARedPowerCylinder>(ActorToCheck))
	{
		if (OtherCyl != this)
		{
			OutFoundID = OtherCyl->CellID;
			return OutFoundID == CellID;
		}
	}

	if (AActor* ParentActor = ActorToCheck->GetAttachParentActor())
	{
		if (ParentActor != ActorToCheck && CheckActorTargetIDMatch(ParentActor, OutFoundID))
		{
			return true;
		}
	}

	for (const TCHAR* PropName : { TEXT("TargetID"), TEXT("CellID") })
	{
		if (FIntProperty* IntProp = CastField<FIntProperty>(ActorToCheck->GetClass()->FindPropertyByName(PropName)))
		{
			OutFoundID = IntProp->GetPropertyValue_InContainer(ActorToCheck);
			return OutFoundID == CellID;
		}
	}

	return false;
}

void ARedPowerCylinder::RotateGun(float AngleDegrees)
{
	GunYaw = FMath::Fmod(GunYaw + AngleDegrees, 360.0f);
	BuildGunComponents();
	UpdateLaserBeam();
}

void ARedPowerCylinder::ClearDetectedTargets()
{
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
}

AActor* ARedPowerCylinder::TraceLaserBeam(float& OutDistance, bool& bOutHitObstacle) const
{
	OutDistance = LaserLength;
	bOutHitObstacle = false;

	UWorld* World = GetWorld();
	if (!World || !LaserEmitter)
	{
		return nullptr;
	}

	const FVector Start = LaserEmitter->GetComponentLocation();
	const FVector Forward = LaserEmitter->GetForwardVector();
	const FVector DesiredEnd = Start + Forward * LaserLength;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (InsertedBattery)
	{
		Params.AddIgnoredActor(InsertedBattery);
	}

	FHitResult Hit;
	AActor* HitActor = nullptr;
	if (World->LineTraceSingleByChannel(Hit, Start, DesiredEnd, ECC_Visibility, Params))
	{
		HitActor = Hit.GetActor();
		if (bLaserStopsAtObstacles)
		{
			OutDistance = FMath::Max(15.0f, Hit.Distance);
			bOutHitObstacle = true;
		}
	}

	if (!World->IsGameWorld() && bPreviewLaserInEditor)
	{
		const FVector EndPoint = Start + Forward * OutDistance;
		const FColor BeamColor = GetColorForID(CellID).ToFColor(true);
		DrawDebugLine(World, Start, EndPoint, BeamColor, false, 0.0f, 0, FMath::Max(2.0f, LaserThickness * 0.5f));
		if (bOutHitObstacle)
		{
			DrawDebugSphere(World, EndPoint, FMath::Max(6.0f, LaserThickness * 0.75f), 8, BeamColor, false, 0.0f);
		}
	}

	return HitActor;
}

void ARedPowerCylinder::ApplyLaserBurnToPlayer(AActor* HitActor)
{
	AGAME3023_F26_RoshanCharacter* HitPlayer = LiveWireUtils::ResolveActorOrParent<AGAME3023_F26_RoshanCharacter>(HitActor);
	if (!HitPlayer || HitPlayer->IsDead() || !GetWorld())
	{
		PlayerLaserDamageTimer = 0.35f;
		return;
	}

	PlayerLaserDamageTimer += GetWorld()->GetDeltaSeconds();
	if (PlayerLaserDamageTimer >= 0.35f)
	{
		PlayerLaserDamageTimer = 0.0f;
		HitPlayer->ApplyDamageToPlayer(10.0f, this);
	}
}

void ARedPowerCylinder::UpdateDetectedTargets(AActor* HitActor)
{
	int32 FoundTargetID = -1;
	const bool bHitMatchingTarget = CheckActorTargetIDMatch(HitActor, FoundTargetID);
	bIsTargetDetected = bHitMatchingTarget;
	DetectedTarget = bHitMatchingTarget ? HitActor : nullptr;

	ALaserTarget* CurrentHitLaserTarget = LiveWireUtils::ResolveActorOrParent<ALaserTarget>(HitActor);
	ATransformerBushing* CurrentHitBushing = LiveWireUtils::ResolveActorOrParent<ATransformerBushing>(HitActor);

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

		bHasPrintedNoTarget = false;
		if (!bHasPrintedTargetDetected)
		{
			bHasPrintedTargetDetected = true;
			const FString TargetName = HitActor ? HitActor->GetActorNameOrLabel() : TEXT("Target");
			UKismetSystemLibrary::PrintString(this,
				FString::Printf(TEXT("[TARGET MATCH: %d] Laser hit matching target object: %s"), CellID, *TargetName),
				true, true, FLinearColor(0.1f, 1.0f, 0.2f), 3.0f);
		}
	}
	else
	{
		bHasPrintedTargetDetected = false;
		if (!bHasPrintedNoTarget)
		{
			bHasPrintedNoTarget = true;
			UKismetSystemLibrary::PrintString(this,
				FString::Printf(TEXT("[LASER EMITTING] Laser beam emitted (ID: %d) - No matching target detected"), CellID),
				true, true, FLinearColor(1.0f, 0.85f, 0.1f), 2.5f);
		}
	}
}

void ARedPowerCylinder::UpdateLaserBeam()
{
	if (!LaserEmitter)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const bool bIsGame = World && World->IsGameWorld();
	const bool bShouldBeActive = (bIsPowered && bHasBattery) || (bPreviewLaserInEditor && World && !bIsGame);

	if (!bShouldBeActive)
	{
		if (LaserBeamMesh)
		{
			LaserBeamMesh->SetVisibility(false);
		}
		ClearDetectedTargets();
		return;
	}

	float ActualDistance = LaserLength;
	bool bHitObstacle = false;
	AActor* HitActor = TraceLaserBeam(ActualDistance, bHitObstacle);

	if (LaserBeamMesh)
	{
		LaserBeamMesh->SetVisibility(true);
		LaserBeamMesh->SetRelativeLocation(FVector(ActualDistance * 0.5f, 0.0f, 0.0f));
		LaserBeamMesh->SetRelativeRotation(FRotator::ZeroRotator);
		LaserBeamMesh->SetRelativeScale3D(FVector(ActualDistance / 100.0f, LaserThickness / 100.0f, LaserThickness / 100.0f));
	}

	if (bIsGame)
	{
		ApplyLaserBurnToPlayer(HitActor);
		UpdateDetectedTargets(HitActor);
	}
}

bool ARedPowerCylinder::ShouldTickIfViewportsOnly() const
{
	return bPreviewLaserInEditor;
}

void ARedPowerCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	const bool bIsGame = World && World->IsGameWorld();
	const bool bIsEditorPreview = bPreviewLaserInEditor && World && !bIsGame;

	if ((bIsPowered && bHasBattery) || bIsEditorPreview)
	{
		UpdateLaserBeam();
	}

	if (bIsGame && bIsPowered && bHasBattery)
	{
		PowerDomeBlinkTimer += DeltaTime;
		if (PowerDomeBlinkTimer >= FMath::Max(0.05f, PowerDomeBlinkInterval))
		{
			PowerDomeBlinkTimer = 0.0f;
			bPowerDomeLightOn = !bPowerDomeLightOn;
			ApplyPowerDomeLightState(bPowerDomeLightOn);
		}
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

	if (PlayerLoc.Z < (CylinderLoc.Z + 20.0f) || PlayerLoc.Z > (CylinderLoc.Z + 400.0f))
	{
		return false;
	}

	if (FVector::DistSquared2D(PlayerLoc, CylinderLoc) > FMath::Square(280.0f))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

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

	return true;
}

void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor);
	if (!Player || !CanPlayerInteract(Player))
	{
		return;
	}

	Player->SetCurrentInteractable(this);

	if (bHasBattery)
	{
		LiveWireUtils::ShowScreenMessage(2001, 2.5f, FColor::Cyan,
			FString::Printf(TEXT("[Laser Gun] Press [Square] to Remove Battery (ID: %d)"), CellID));
	}
	else if (Player->HasBattery())
	{
		LiveWireUtils::ShowScreenMessage(2001, 2.5f, FColor::Green,
			FString::Printf(TEXT("[Laser Gun] Press [Square] to Attach Battery into Laser Gun (ID: %d)"), CellID));
	}
	else
	{
		LiveWireUtils::ShowScreenMessage(2001, 2.5f, FColor::Orange,
			FString::Printf(TEXT("[Laser Gun] Laser Gun (ID: %d): OFFLINE (Requires Matching Battery)"), CellID));
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
	if (!Player || !CanPlayerInteract(Player))
	{
		return false;
	}

	if (bHasBattery)
	{
		if (Player->HasBattery())
		{
			LiveWireUtils::ShowScreenMessage(-1, 3.5f, FColor::Red,
				TEXT("[INVENTORY FULL] Hands full! Press [R] / [Circle] to drop your current battery first."), 1.3f);
			return false;
		}

		if (ABatteryCapsule* Extracted = RemoveBattery())
		{
			Player->TryPickupBattery(Extracted);
			LiveWireUtils::ShowScreenMessage(-1, 4.0f, FColor::Yellow,
				FString::Printf(TEXT("[BATTERY REMOVED] Battery removed! Laser Gun (ID: %d) is now OFFLINE."), CellID), 1.3f);
			return true;
		}
		return false;
	}

	if (!Player->HasBattery())
	{
		LiveWireUtils::ShowScreenMessage(-1, 3.5f, FColor::Red,
			FString::Printf(TEXT("[POWER OFFLINE] Laser Gun (ID: %d) needs a matching Battery!"), CellID), 1.3f);
		return false;
	}

	const bool bMatched = InsertBattery(Player->TakeCarriedBattery());
	Player->TriggerControllerRumble(bMatched ? 1.0f : 0.6f, bMatched ? 0.35f : 0.15f, true, bMatched);
	return bMatched;
}

ABatteryCapsule* ARedPowerCylinder::RemoveBattery()
{
	if (!bHasBattery || !InsertedBattery)
	{
		return nullptr;
	}

	ABatteryCapsule* Extracted = InsertedBattery;
	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;

	ClearDetectedTargets();
	UpdateVisualState();
	UpdateLaserBeam();

	PowerDomeBlinkTimer = 0.0f;
	bPowerDomeLightOn = false;
	ApplyPowerDomeLightState(false);

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery)
	{
		return false;
	}

	InsertedBattery = Battery;
	bHasBattery = true;
	Battery->OnSocketed(this, BatterySocket);

	const bool bMatched = (Battery->BatteryID == CellID);
	bIsPowered = bMatched;
	bHasPrintedNoTarget = false;
	bHasPrintedTargetDetected = false;
	bIsTargetDetected = false;

	UpdateVisualState();
	UpdateLaserBeam();

	if (bMatched)
	{
		PowerDomeBlinkTimer = 0.0f;
		bPowerDomeLightOn = true;
		ApplyPowerDomeLightState(true);

		LiveWireUtils::ShowScreenMessage(-1, 4.5f, FColor::Green,
			FString::Printf(TEXT("[ID MATCH: %d] Battery attached & Laser Gun ONLINE! Laser beam active!"), CellID), 1.4f);
	}
	else
	{
		LiveWireUtils::ShowScreenMessage(-1, 5.0f, FColor::Orange,
			FString::Printf(TEXT("[ID MISMATCH] Battery (ID: %d) attached, but Laser Gun (ID: %d) WON'T POWER ON!"), Battery->BatteryID, CellID), 1.4f);
	}

	return bMatched;
}

void ARedPowerCylinder::UpdateVisualState()
{
	ApplyColorFromID();
}
