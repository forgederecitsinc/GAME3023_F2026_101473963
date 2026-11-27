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
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetSystemLibrary.h"

// Constructor: creates the cylinder, battery socket, laser emitter, and interaction zone.
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

// OnConstruction: updates editor preview visuals and the laser beam.
void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateLaserBeam();
}

#if WITH_EDITOR
// PostEditChangeProperty: refreshes colors and laser preview after an editor property changes.
void ARedPowerCylinder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateLaserBeam();
}
#endif

// BeginPlay: connects overlap events and initializes the cylinder visuals.
void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	InitPowerDome();
	UpdateLaserBeam();
}

// CreatePowerDomeMaterial: creates a dynamic emissive material for the power dome.
UMaterialInstanceDynamic* ARedPowerCylinder::CreatePowerDomeMaterial(const FLinearColor& BaseColor)
{
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BaseMat)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
	if (DynMat)
	{
		const FLinearColor EmissiveColor = BaseColor * PowerDomeGlowIntensity;
		DynMat->SetVectorParameterValue(TEXT("Color"), BaseColor);
		DynMat->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
		DynMat->SetVectorParameterValue(TEXT("Base Color"), BaseColor);
		DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		DynMat->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
		DynMat->SetVectorParameterValue(TEXT("Emissive"), EmissiveColor);
	}
	return DynMat;
}

// InitPowerDome: finds the dome mesh and creates its materials and blinking light.
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
		PowerDomeGreenDynMaterial = CreatePowerDomeMaterial(FLinearColor(0.05f, 1.0f, 0.15f, 1.0f));
	}

	if (!PowerDomeRedDynMaterial)
	{
		PowerDomeRedDynMaterial = CreatePowerDomeMaterial(FLinearColor(1.0f, 0.05f, 0.05f, 1.0f));
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

	bPowerDomeLightOn = bHasBattery;
	ApplyPowerDomeLightState(bPowerDomeLightOn);
}

// ApplyPowerDomeLightState: switches the dome between green, red, and inactive states.
void ARedPowerCylinder::ApplyPowerDomeLightState(bool bLightOn)
{
	const bool bPoweredActive = (bIsPowered && bHasBattery);
	const bool bMismatchActive = (!bIsPowered && bHasBattery);
	const bool bShowGreen = bPoweredActive && bLightOn;
	const bool bShowRed = bMismatchActive && bLightOn;

	if (PowerDomeMesh)
	{
		for (int32 i = 0; i < PowerDomeOriginalMaterials.Num(); ++i)
		{
			if (bShowGreen && PowerDomeGreenDynMaterial)
			{
				PowerDomeMesh->SetMaterial(i, PowerDomeGreenDynMaterial);
			}
			else if (bShowRed && PowerDomeRedDynMaterial)
			{
				PowerDomeMesh->SetMaterial(i, PowerDomeRedDynMaterial);
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
		PowerDomeLight->SetLightColor(bShowGreen ? FColor(20, 255, 50) : (bShowRed ? FColor(255, 25, 25) : FColor(255, 130, 15)));
		PowerDomeLight->SetVisibility(bPoweredActive || bMismatchActive);
	}
}

// GetColorForID: returns the LiveWire color mapped to the cylinder ID.
FLinearColor ARedPowerCylinder::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

// CheckActorTargetIDMatch: checks whether a hit actor has a matching four-digit ID.
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

// ClearDetectedTargets: clears target detection and disconnects the previous laser hits.
void ARedPowerCylinder::ClearDetectedTargets()
{
	bIsTargetDetected = false;
	DetectedTarget = nullptr;

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

// TraceLaserBeam: fires a line trace from the laser emitter and returns the hit actor.
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

	return HitActor;
}

// ApplyLaserBurnToPlayer: damages the player while they remain in the laser beam.
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

// UpdateDetectedTargets: activates matching laser targets and transformer bushings.
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
	}
}

// UpdateLaserBeam: scales the beam to the hit distance and updates powered targets.
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

// ShouldTickIfViewportsOnly: allows the laser preview to update outside game mode.
bool ARedPowerCylinder::ShouldTickIfViewportsOnly() const
{
	return bPreviewLaserInEditor;
}

// Tick: updates the laser in preview or gameplay and blinks the dome when powered.
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

	if (bIsGame && bHasBattery)
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

// CanPlayerInteract: checks height, distance, and line of sight to the player.
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

// OnInteractionBeginOverlap: shows an insert or remove battery prompt near the player.
void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor);
	if (!Player || !CanPlayerInteract(Player))
	{
		return;
	}

	Player->SetCurrentInteractable(this);
}

// OnInteractionEndOverlap: removes this cylinder as the player's current interactable.
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

// Interact: inserts or removes the battery when the player uses the cylinder.
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
			return false;
		}

		if (ABatteryCapsule* Extracted = RemoveBattery())
		{
			Player->TryPickupBattery(Extracted);
			return true;
		}
		return false;
	}

	if (!Player->HasBattery())
	{
		return false;
	}

	const bool bMatched = InsertBattery(Player->TakeCarriedBattery());
	Player->TriggerControllerRumble(bMatched ? 1.0f : 0.6f, bMatched ? 0.35f : 0.15f, true, bMatched);
	return bMatched;
}

// RemoveBattery: removes the battery, disconnects the laser, and resets the dome.
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
	UpdateLaserBeam();

	PowerDomeBlinkTimer = 0.0f;
	bPowerDomeLightOn = false;
	ApplyPowerDomeLightState(false);

	return Extracted;
}

// InsertBattery: installs a battery and enables power only when the IDs match.
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
	bIsTargetDetected = false;

	UpdateLaserBeam();

	PowerDomeBlinkTimer = 0.0f;
	bPowerDomeLightOn = true;
	ApplyPowerDomeLightState(true);

	return bMatched;
}
