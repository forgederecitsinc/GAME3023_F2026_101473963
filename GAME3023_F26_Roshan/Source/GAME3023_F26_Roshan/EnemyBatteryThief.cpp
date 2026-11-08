// roshan - 101473963 - game3023 game engines 3
// ground battery saboteur / battery thief enemy implementation
// spawns at (X=-878.0, Y=-3679.0, Z=10.0) at speed 300, patrols the ground NavMesh, ignores loose or mismatched
// batteries, detects ONLY batteries that work with a laser (bIsPowered && bHasBattery), navigates to
// the nearest active laser battery, runs a 10-second removal timer to detach the battery, scans the NavMesh
// for the closest placed BP_BatteryDropSpot actor in the level, carries the battery there on its back, drops it, and returns to patrol

#include "EnemyBatteryThief.h"
#include "RedPowerCylinder.h"
#include "BatteryCapsule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"

AEnemyBatteryThief::AEnemyBatteryThief()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root scene component at ground level
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Low-profile ground spider/crawler mesh (can be overridden or hidden in BP_EnemyBatteryThief)
	ThiefMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThiefMesh"));
	ThiefMesh->SetupAttachment(SceneRoot);
	ThiefMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	ThiefMesh->SetRelativeScale3D(BodyMeshScale);
	ThiefMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	ThiefMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ThiefMesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		ThiefMesh->SetStaticMesh(CubeFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (FlatMatFinder.Succeeded())
	{
		ThiefMesh->SetMaterial(0, FlatMatFinder.Object);
	}

	// Automatically bind BP_BatteryDropSpot as the default drop spot actor class
	static ConstructorHelpers::FClassFinder<AActor> DropSpotClassFinder(TEXT("/Game/_Blueprints/LevelAssets/BP_BatteryDropSpot"));
	if (DropSpotClassFinder.Succeeded())
	{
		BatteryDropSpotClass = DropSpotClassFinder.Class;
	}

	// Socket on top of the chassis where a stolen battery snaps after removal completes
	CarriedBatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("CarriedBatterySocket"));
	CarriedBatterySocket->SetupAttachment(SceneRoot);
	CarriedBatterySocket->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));

	// Forward-facing search light matching the Ground Battery Saboteur spec
	ScannerLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("ScannerLight"));
	ScannerLight->SetupAttachment(SceneRoot);
	ScannerLight->SetRelativeLocation(FVector(35.0f, 0.0f, 25.0f));
	ScannerLight->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f));
	ScannerLight->Intensity = ScannerLightIntensity;
	ScannerLight->SetLightColor(ScannerLightColor);
	ScannerLight->InnerConeAngle = ScannerOuterConeAngle * 0.6f;
	ScannerLight->OuterConeAngle = ScannerOuterConeAngle;
	ScannerLight->AttenuationRadius = ScannerLightRange;
	ScannerLight->SetCastShadows(false);

	// Ground movement component
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = SceneRoot;
	MovementComponent->MaxSpeed = PatrolSpeed;
	MovementComponent->Acceleration = 1200.0f;
	MovementComponent->Deceleration = 2000.0f;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyBatteryThief::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEnemySettings();
}

#if WITH_EDITOR
void AEnemyBatteryThief::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEnemySettings();
}
#endif

void AEnemyBatteryThief::RefreshEnemySettings()
{
	if (ThiefMesh)
	{
		ThiefMesh->SetRelativeScale3D(BodyMeshScale);
	}

	if (ScannerLight)
	{
		ScannerLight->SetIntensity(ScannerLightIntensity);
		const FLinearColor ActiveColor = (CurrentState == EBatteryThiefState::Approaching || CurrentState == EBatteryThiefState::Removing)
			? AlertLightColor
			: ScannerLightColor;
		ScannerLight->SetLightColor(ActiveColor);
		ScannerLight->SetInnerConeAngle(ScannerOuterConeAngle * 0.6f);
		ScannerLight->SetOuterConeAngle(ScannerOuterConeAngle);
		ScannerLight->SetAttenuationRadius(ScannerLightRange);
	}

	if (MovementComponent)
	{
		MovementComponent->MaxSpeed = (CurrentState == EBatteryThiefState::Carrying) ? CarrySpeed : PatrolSpeed;
	}
}

void AEnemyBatteryThief::MarkAsSpawnedUnit(const FVector& InHomeOrigin)
{
	bIsSpawnedChildUnit = true;
	bActAsSpawner = false;
	bSnapToSpawnLocationOnStart = false;

	// Lock ground floor Z to the ground NavMesh at the spawn origin (Z ~ 10)
	GroundFloorZ = InHomeOrigin.Z;
	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedSpawn;
			if (NavSys->ProjectPointToNavigation(InHomeOrigin, ProjectedSpawn, FVector(300.0f, 300.0f, 80.0f)))
			{
				GroundFloorZ = ProjectedSpawn.Location.Z;
			}
		}
	}

	CurrentGroundZ = GroundFloorZ;
	bHasValidGroundZ = true;
	PatrolOrigin = FVector(InHomeOrigin.X, InHomeOrigin.Y, GroundFloorZ);
	SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ + GroundHeightOffset));
}

void AEnemyBatteryThief::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	bIsDead = false;

	// Ensure existing Blueprint assets that saved the earlier 260/160 default speed use the requested 300 speed
	if (FMath::IsNearlyEqual(PatrolSpeed, 260.0f))
	{
		PatrolSpeed = 300.0f;
	}
	if (FMath::IsNearlyEqual(CarrySpeed, 160.0f))
	{
		CarrySpeed = 300.0f;
	}

	RefreshEnemySettings();

	// If configured to snap to the target spawn coordinates (X=-878, Y=-3679, Z=10)
	if (!bIsSpawnedChildUnit && bSnapToSpawnLocationOnStart)
	{
		SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	}

	const FVector StartPos = GetActorLocation();
	const FVector GroundReferencePos(StartPos.X, StartPos.Y, SpawnLocation.Z);

	GroundFloorZ = SpawnLocation.Z;
	if (UWorld* World = GetWorld())
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedFloor;
			if (NavSys->ProjectPointToNavigation(GroundReferencePos, ProjectedFloor, FVector(300.0f, 300.0f, 80.0f)))
			{
				GroundFloorZ = ProjectedFloor.Location.Z;
			}
		}
	}

	CurrentGroundZ = GroundFloorZ;
	bHasValidGroundZ = true;

	if (!bIsSpawnedChildUnit)
	{
		// Use the actor's placed/spawned location on the ground floor as the NavMesh patrol origin
		PatrolOrigin = FVector(StartPos.X, StartPos.Y, GroundFloorZ);
	}

	// Snap cleanly onto the ground NavMesh surface — never above ground level
	SetActorLocation(FVector(StartPos.X, StartPos.Y, GroundFloorZ + GroundHeightOffset));

	// Start patrolling the ground NavMesh right away
	CurrentState = EBatteryThiefState::Patrolling;
	PickNextNavMeshPatrolDestination();

	// If periodic spawning out of SpawnLocation is enabled on this instance, schedule the spawn timer
	if (!bIsSpawnedChildUnit && bActAsSpawner && MaxEnemiesToSpawn > 0)
	{
		if (UWorld* World = GetWorld())
		{
			const float FirstDelay = FMath::Max(0.05f, InitialSpawnDelay);
			World->GetTimerManager().SetTimer(
				SpawnTimerHandle,
				[this]()
				{
					SpawnThiefAtTargetLocation();
				},
				SpawnInterval,
				true,
				FirstDelay
			);
		}
	}
}

void AEnemyBatteryThief::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	// If destroyed while carrying a stolen battery, safely drop the battery onto the ground so the puzzle is never unwinnable
	if (IsValid(StolenBattery))
	{
		const FVector SafeDropPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);
		StolenBattery->OnDropped(SafeDropPos);
		StolenBattery = nullptr;
	}

	if (!bIsDead && EndPlayReason == EEndPlayReason::Destroyed)
	{
		bIsDead = true;
		OnBatteryThiefKilled.Broadcast(this);
	}

	Super::EndPlay(EndPlayReason);
}

float AEnemyBatteryThief::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	ApplyDamage(ActualDamage, DamageCauser, GetActorLocation(), FVector::ZeroVector);
	return ActualDamage;
}

void AEnemyBatteryThief::ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse)
{
	if (bIsDead || Damage <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath();
	}
}

void AEnemyBatteryThief::HandleDeath()
{
	KillBatteryThief();
}

void AEnemyBatteryThief::ApplyHealing(float Healing, AActor* Healer)
{
	if (bIsDead || Healing <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Healing, 0.0f, MaxHealth);
}

void AEnemyBatteryThief::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
	// No-op for Battery Thief
}

void AEnemyBatteryThief::KillBatteryThief()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	// Safely drop any carried battery at our current ground position so the player can recover it
	if (IsValid(StolenBattery))
	{
		const FVector SafeDropPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);
		StolenBattery->OnDropped(SafeDropPos);
		StolenBattery = nullptr;
	}

	TargetLaserCylinder = nullptr;
	TargetDropSpotActor = nullptr;

	// Notify the GameMode / Spawner so a replacement spawns in 60 seconds
	OnBatteryThiefKilled.Broadcast(this);

	Destroy();
}

AEnemyBatteryThief* AEnemyBatteryThief::SpawnThiefAtTargetLocation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Count currently living Battery Thieves in the NavMesh so at most MaxEnemiesToSpawn (5) stay inside at all times
	TArray<AActor*> LivingThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), LivingThieves);
	int32 ActiveCount = 0;
	for (AActor* Act : LivingThieves)
	{
		AEnemyBatteryThief* Existing = Cast<AEnemyBatteryThief>(Act);
		if (IsValid(Existing) && !Existing->bIsDead)
		{
			++ActiveCount;
		}
	}

	if (ActiveCount >= MaxEnemiesToSpawn)
	{
		return nullptr;
	}

	TSubclassOf<AEnemyBatteryThief> ClassToSpawn = EnemyClassToSpawn ? *EnemyClassToSpawn : GetClass();
	const float GroundZAtSpawn = SampleGroundNavMeshZ(SpawnLocation);
	const FVector FinalSpawnPos(SpawnLocation.X, SpawnLocation.Y, GroundZAtSpawn + GroundHeightOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	// AlwaysSpawn prevents Unreal from pushing a newly spawned spider thief up onto an adjacent wall/box
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyBatteryThief* NewThief = World->SpawnActor<AEnemyBatteryThief>(ClassToSpawn, FinalSpawnPos, SpawnRotation, SpawnParams);
	if (NewThief)
	{
		++TotalSpawnedCount;
		NewThief->PatrolSpeed = PatrolSpeed;
		NewThief->CarrySpeed = CarrySpeed;
		NewThief->TurnSpeed = TurnSpeed;
		NewThief->GroundHeightOffset = GroundHeightOffset;
		NewThief->MaxGroundStepHeight = MaxGroundStepHeight;
		NewThief->GroundCollisionRadius = GroundCollisionRadius;
		NewThief->PatrolRadius = PatrolRadius;
		NewThief->AcceptanceRadius = AcceptanceRadius;
		NewThief->PatrolPointWaitTime = PatrolPointWaitTime;
		NewThief->BatteryDetectionRadius = BatteryDetectionRadius;
		NewThief->RemovalInteractionDistance = RemovalInteractionDistance;
		NewThief->BatteryRemovalDuration = BatteryRemovalDuration;
		NewThief->BatteryDropSpotClass = BatteryDropSpotClass;
		NewThief->DropOffAcceptanceRadius = DropOffAcceptanceRadius;
		NewThief->CustomPatrolWaypoints = CustomPatrolWaypoints;
		NewThief->RefreshEnemySettings();
		NewThief->MarkAsSpawnedUnit(FVector(SpawnLocation.X, SpawnLocation.Y, GroundZAtSpawn));
		NewThief->PickNextNavMeshPatrolDestination();
	}

	return NewThief;
}

bool AEnemyBatteryThief::IsPointOnGroundFloor(const FVector& Point) const
{
	return FMath::Abs(Point.Z - GroundFloorZ) <= MaxGroundStepHeight;
}

bool AEnemyBatteryThief::IsGroundNavPathValid(const UNavigationPath* NavPath) const
{
	if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial() || NavPath->PathPoints.Num() < 2)
	{
		return false;
	}

	// Every path corner must stay strictly on the ground floor surface (never climbing onto walls, boxes, or pedestals)
	for (const FVector& Pt : NavPath->PathPoints)
	{
		if (!IsPointOnGroundFloor(Pt))
		{
			return false;
		}
	}

	return true;
}

FVector AEnemyBatteryThief::GetCylinderTriggerGroundCenter(const ARedPowerCylinder* Cylinder) const
{
	if (!IsValid(Cylinder))
	{
		return FVector(0.0f, 0.0f, GroundFloorZ);
	}

	// If the cylinder has an InteractionZone or any trigger box/sphere component (even if placed above ground level),
	// project its horizontal (X, Y) center straight down to the ground floor level (GroundFloorZ)
	if (IsValid(Cylinder->InteractionZone))
	{
		const FVector ZoneLoc = Cylinder->InteractionZone->GetComponentLocation();
		return FVector(ZoneLoc.X, ZoneLoc.Y, GroundFloorZ);
	}

	TInlineComponentArray<UShapeComponent*> ShapeComponents(Cylinder);
	for (const UShapeComponent* ShapeComp : ShapeComponents)
	{
		if (IsValid(ShapeComp))
		{
			const FVector ShapeLoc = ShapeComp->GetComponentLocation();
			return FVector(ShapeLoc.X, ShapeLoc.Y, GroundFloorZ);
		}
	}

	const FVector CylLoc = Cylinder->GetActorLocation();
	return FVector(CylLoc.X, CylLoc.Y, GroundFloorZ);
}

bool AEnemyBatteryThief::ComputeGroundStealApproachPoint(const ARedPowerCylinder* Cylinder, FVector& OutGroundPoint, float& OutNavPathLength) const
{
	OutGroundPoint = FVector::ZeroVector;
	OutNavPathLength = TNumericLimits<float>::Max();

	UWorld* World = GetWorld();
	if (!World || !IsValid(Cylinder))
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return false;
	}

	const FVector MyGroundPos(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ);
	FVector PathStartPos = MyGroundPos;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(MyGroundPos, ProjectedStart, FVector(300.0f, 300.0f, MaxGroundStepHeight)) &&
		IsPointOnGroundFloor(ProjectedStart.Location))
	{
		PathStartPos = ProjectedStart.Location;
	}

	// Project the cylinder's trigger box center down onto the ground floor (Z = GroundFloorZ)
	const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(Cylinder);
	const FVector ToThief2D = (MyGroundPos - TriggerGroundCenter).GetSafeNormal2D();
	const float BaseAngleRad = ToThief2D.IsNearlyZero() ? 0.0f : FMath::Atan2(ToThief2D.Y, ToThief2D.X);

	// Build candidate ground points directly beneath the trigger box and in rings around the pedestal base on the ground floor
	TArray<FVector, TInlineAllocator<36>> CandidateGroundPoints;
	CandidateGroundPoints.Add(TriggerGroundCenter);

	const float RingRadii[] = { 90.0f, 150.0f, 210.0f, 270.0f };
	const float AngleOffsetsDeg[] = { 0.0f, 45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f, 180.0f };

	for (float Radius : RingRadii)
	{
		for (float OffsetDeg : AngleOffsetsDeg)
		{
			const float AngleRad = BaseAngleRad + FMath::DegreesToRadians(OffsetDeg);
			CandidateGroundPoints.Add(FVector(
				TriggerGroundCenter.X + FMath::Cos(AngleRad) * Radius,
				TriggerGroundCenter.Y + FMath::Sin(AngleRad) * Radius,
				GroundFloorZ
			));
		}
	}

	const float MaxAllowedReach2D = FMath::Max(RemovalInteractionDistance + 110.0f, 340.0f);
	bool bFoundValidGroundRoute = false;

	for (const FVector& Candidate : CandidateGroundPoints)
	{
		FNavLocation ProjectedTarget;
		if (!NavSys->ProjectPointToNavigation(Candidate, ProjectedTarget, FVector(110.0f, 110.0f, MaxGroundStepHeight)))
		{
			continue;
		}

		// Must be strictly on the ground level
		if (!IsPointOnGroundFloor(ProjectedTarget.Location))
		{
			continue;
		}

		// Must be close enough horizontally to the cylinder's trigger box on the ground to steal the battery
		if (FVector::Dist2D(ProjectedTarget.Location, TriggerGroundCenter) > MaxAllowedReach2D)
		{
			continue;
		}

		// If the thief is already standing at this ground approach point
		const float Dist2DFromStart = FVector::Dist2D(PathStartPos, ProjectedTarget.Location);
		if (Dist2DFromStart <= FMath::Max(AcceptanceRadius, 45.0f))
		{
			OutGroundPoint = FVector(ProjectedTarget.Location.X, ProjectedTarget.Location.Y, GroundFloorZ);
			OutNavPathLength = Dist2DFromStart;
			return true;
		}

		UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
			World,
			PathStartPos,
			ProjectedTarget.Location,
			const_cast<AEnemyBatteryThief*>(this)
		);

		if (IsGroundNavPathValid(NavPath))
		{
			const float PathLen = NavPath->GetPathLength();
			if (PathLen < OutNavPathLength)
			{
				OutNavPathLength = PathLen;
				OutGroundPoint = FVector(ProjectedTarget.Location.X, ProjectedTarget.Location.Y, GroundFloorZ);
				bFoundValidGroundRoute = true;
			}
		}
	}

	return bFoundValidGroundRoute;
}

bool AEnemyBatteryThief::IsNearestThiefForCylinder(const ARedPowerCylinder* Cylinder, float MyGroundPathLength) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Cylinder))
	{
		return false;
	}

	// Small hysteresis advantage if this thief is already actively approaching this cylinder
	const bool bAmAlreadyApproachingThis = (CurrentState == EBatteryThiefState::Approaching && TargetLaserCylinder == Cylinder);
	const float MyEffectiveCost = MyGroundPathLength - (bAmAlreadyApproachingThis ? 45.0f : 0.0f);

	TArray<AActor*> AllThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), AllThieves);

	for (AActor* Act : AllThieves)
	{
		const AEnemyBatteryThief* Other = Cast<AEnemyBatteryThief>(Act);
		if (!IsValid(Other) || Other == this || Other->bIsDead)
		{
			continue;
		}

		// If another thief is already removing the battery on this cylinder, no other thief may target it
		if (Other->CurrentState == EBatteryThiefState::Removing && Other->TargetLaserCylinder == Cylinder)
		{
			return false;
		}

		// Thieves carrying a stolen battery or busy removing/approaching a DIFFERENT working cylinder are occupied
		if (Other->CurrentState == EBatteryThiefState::Carrying)
		{
			continue;
		}
		if ((Other->CurrentState == EBatteryThiefState::Removing || Other->CurrentState == EBatteryThiefState::Approaching) &&
			IsValid(Other->TargetLaserCylinder) &&
			Other->TargetLaserCylinder != Cylinder &&
			DoesLaserHaveWorkingBattery(Other->TargetLaserCylinder))
		{
			continue;
		}

		FVector OtherGroundPt = FVector::ZeroVector;
		float OtherPathLength = TNumericLimits<float>::Max();
		if (Other->ComputeGroundStealApproachPoint(Cylinder, OtherGroundPt, OtherPathLength))
		{
			const bool bOtherApproachingThis = (Other->CurrentState == EBatteryThiefState::Approaching && Other->TargetLaserCylinder == Cylinder);
			const float OtherEffectiveCost = OtherPathLength - (bOtherApproachingThis ? 45.0f : 0.0f);

			if (OtherEffectiveCost < MyEffectiveCost - 1.0f ||
				(FMath::IsNearlyEqual(OtherEffectiveCost, MyEffectiveCost, 1.0f) && Other < this))
			{
				return false;
			}
		}
	}

	return true;
}

bool AEnemyBatteryThief::DoesLaserHaveWorkingBattery(const ARedPowerCylinder* Cylinder) const
{
	if (!IsValid(Cylinder))
	{
		return false;
	}

	// Only detect batteries that actually work with the laser and activated the laser beam!
	// If a battery is loose on the floor or has a mismatched ID (bIsPowered == false), ignore it.
	return Cylinder->bHasBattery &&
		Cylinder->bIsPowered &&
		IsValid(Cylinder->InsertedBattery) &&
		(Cylinder->InsertedBattery->BatteryID == Cylinder->CellID);
}

ARedPowerCylinder* AEnemyBatteryThief::FindNearestWorkingBatteryLaser() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundCylinders;
	UGameplayStatics::GetAllActorsOfClass(World, ARedPowerCylinder::StaticClass(), FoundCylinders);

	const FVector MyLoc = GetActorLocation();
	const float MaxDistSq = FMath::Square(BatteryDetectionRadius);

	ARedPowerCylinder* NearestValidLaser = nullptr;
	float BestGroundPathLength = TNumericLimits<float>::Max();

	for (AActor* Actor : FoundCylinders)
	{
		ARedPowerCylinder* Cylinder = Cast<ARedPowerCylinder>(Actor);
		if (!DoesLaserHaveWorkingBattery(Cylinder))
		{
			continue;
		}

		const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(Cylinder);
		if (FVector::DistSquared2D(MyLoc, TriggerGroundCenter) > MaxDistSq)
		{
			continue;
		}

		// Calculate a route strictly through the ground level to steal from this cylinder
		FVector CandidateGroundPoint = FVector::ZeroVector;
		float CandidatePathLength = TNumericLimits<float>::Max();
		if (!ComputeGroundStealApproachPoint(Cylinder, CandidateGroundPoint, CandidatePathLength))
		{
			continue;
		}

		// Only the single nearest pawn on the ground level is allowed to try to steal this battery
		if (!IsNearestThiefForCylinder(Cylinder, CandidatePathLength))
		{
			continue;
		}

		if (CandidatePathLength < BestGroundPathLength)
		{
			BestGroundPathLength = CandidatePathLength;
			NearestValidLaser = Cylinder;
		}
	}

	return NearestValidLaser;
}

AActor* AEnemyBatteryThief::FindClosestBatteryDropSpot() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> CandidateDropSpots;

	// 1. Find all actors of BatteryDropSpotClass (BP_BatteryDropSpot)
	if (BatteryDropSpotClass)
	{
		UGameplayStatics::GetAllActorsOfClass(World, BatteryDropSpotClass, CandidateDropSpots);
	}

	// 2. Fallback: also scan for any placed actor whose class or name contains "BatteryDropSpot"
	if (CandidateDropSpots.Num() == 0)
	{
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		for (AActor* Act : AllActors)
		{
			if (!IsValid(Act) || Act == this)
			{
				continue;
			}
			const FString ClassName = Act->GetClass()->GetName();
			const FString ActorName = Act->GetName();
			if (ClassName.Contains(TEXT("BatteryDropSpot")) || ActorName.Contains(TEXT("BatteryDropSpot")))
			{
				CandidateDropSpots.Add(Act);
			}
		}
	}

	if (CandidateDropSpots.Num() == 0)
	{
		return nullptr;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	const FVector MyGroundPos(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ);

	FVector PathStartPos = MyGroundPos;
	if (NavSys)
	{
		FNavLocation ProjectedStart;
		if (NavSys->ProjectPointToNavigation(MyGroundPos, ProjectedStart, FVector(300.0f, 300.0f, MaxGroundStepHeight)) &&
			IsPointOnGroundFloor(ProjectedStart.Location))
		{
			PathStartPos = ProjectedStart.Location;
		}
	}

	AActor* ClosestDropSpot = nullptr;
	float ShortestNavCost = TNumericLimits<float>::Max();

	// Evaluate every BP_BatteryDropSpot on the ground NavMesh and pick the one with the shortest ground path length
	for (AActor* Spot : CandidateDropSpots)
	{
		if (!IsValid(Spot))
		{
			continue;
		}

		const FVector SpotLoc = Spot->GetActorLocation();
		const FVector SpotGroundPos(SpotLoc.X, SpotLoc.Y, GroundFloorZ);
		float CandidateDistanceScore = FVector::Dist2D(MyGroundPos, SpotGroundPos);

		if (NavSys)
		{
			FNavLocation ProjectedNav;
			if (NavSys->ProjectPointToNavigation(SpotGroundPos, ProjectedNav, FVector(350.0f, 350.0f, MaxGroundStepHeight)) &&
				IsPointOnGroundFloor(ProjectedNav.Location))
			{
				UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
					World,
					PathStartPos,
					ProjectedNav.Location,
					const_cast<AEnemyBatteryThief*>(this)
				);
				if (IsGroundNavPathValid(NavPath))
				{
					CandidateDistanceScore = NavPath->GetPathLength();
				}
				else
				{
					CandidateDistanceScore += 50000.0f;
				}
			}
		}

		if (CandidateDistanceScore < ShortestNavCost)
		{
			ShortestNavCost = CandidateDistanceScore;
			ClosestDropSpot = Spot;
		}
	}

	return ClosestDropSpot;
}

float AEnemyBatteryThief::SampleGroundNavMeshZ(const FVector& WorldPos) const
{
	// The spider Battery Thief is strictly locked to the ground surface and cannot climb or fly above GroundFloorZ
	if (bHasValidGroundZ)
	{
		return GroundFloorZ;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return SpawnLocation.Z;
	}

	const FVector GroundQueryPos(WorldPos.X, WorldPos.Y, SpawnLocation.Z);
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedNav;
		if (NavSys->ProjectPointToNavigation(GroundQueryPos, ProjectedNav, FVector(250.0f, 250.0f, 80.0f)))
		{
			return ProjectedNav.Location.Z;
		}
	}

	return SpawnLocation.Z;
}

bool AEnemyBatteryThief::BuildPathToWorldLocation(const FVector& DestinationWorldPos)
{
	CurrentPathPoints.Reset();
	CurrentPathCornerIndex = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return false;
	}

	const FVector CurrentGroundPos(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ);
	FVector PathStartPos = CurrentGroundPos;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(CurrentGroundPos, ProjectedStart, FVector(300.0f, 300.0f, MaxGroundStepHeight)) &&
		IsPointOnGroundFloor(ProjectedStart.Location))
	{
		PathStartPos = ProjectedStart.Location;
	}

	// Project the target destination onto the ground-level NavMesh (Z = GroundFloorZ)
	const FVector GroundDestination(DestinationWorldPos.X, DestinationWorldPos.Y, GroundFloorZ);
	FNavLocation ProjectedTarget;
	if (!NavSys->ProjectPointToNavigation(GroundDestination, ProjectedTarget, FVector(350.0f, 350.0f, MaxGroundStepHeight)) ||
		!IsPointOnGroundFloor(ProjectedTarget.Location))
	{
		return false;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, ProjectedTarget.Location, this);
	if (IsGroundNavPathValid(NavPath))
	{
		CurrentPathPoints.Reserve(NavPath->PathPoints.Num());
		for (const FVector& Pt : NavPath->PathPoints)
		{
			CurrentPathPoints.Add(FVector(Pt.X, Pt.Y, GroundFloorZ));
		}
		CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
		return true;
	}

	return false;
}

void AEnemyBatteryThief::PickNextNavMeshPatrolDestination()
{
	CurrentPathPoints.Reset();
	CurrentPathCornerIndex = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		return;
	}

	const FVector CurrentGroundPos(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ);
	FVector PathStartPos = CurrentGroundPos;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(CurrentGroundPos, ProjectedStart, FVector(300.0f, 300.0f, MaxGroundStepHeight)) &&
		IsPointOnGroundFloor(ProjectedStart.Location))
	{
		PathStartPos = ProjectedStart.Location;
	}

	// 1. If custom patrol waypoints are assigned on the instance, cycle through them using ground-only paths
	if (CustomPatrolWaypoints.Num() > 0)
	{
		const int32 NumWaypoints = CustomPatrolWaypoints.Num();
		for (int32 i = 0; i < NumWaypoints; ++i)
		{
			CustomWaypointIndex = (CustomWaypointIndex + 1) % NumWaypoints;
			AActor* WaypointActor = CustomPatrolWaypoints[CustomWaypointIndex];
			if (!IsValid(WaypointActor))
			{
				continue;
			}

			if (BuildPathToWorldLocation(WaypointActor->GetActorLocation()))
			{
				return;
			}
		}
	}

	// 2. Pick a reachable ground-floor point on the NavMesh within PatrolRadius (rejecting any wall tops or elevated islands)
	FVector SearchOrigin = PathStartPos;
	const FVector GroundPatrolOrigin(PatrolOrigin.X, PatrolOrigin.Y, GroundFloorZ);
	FNavLocation ProjectedOrigin;
	if (NavSys->ProjectPointToNavigation(GroundPatrolOrigin, ProjectedOrigin, FVector(350.0f, 350.0f, MaxGroundStepHeight)) &&
		IsPointOnGroundFloor(ProjectedOrigin.Location))
	{
		SearchOrigin = ProjectedOrigin.Location;
	}

	for (int32 Attempt = 0; Attempt < 24; ++Attempt)
	{
		FNavLocation RandomNavPoint;
		const FVector QueryCenter = (Attempt < 16) ? SearchOrigin : PathStartPos;
		if (NavSys->GetRandomReachablePointInRadius(QueryCenter, PatrolRadius, RandomNavPoint))
		{
			// Reject any point on top of walls, boxes, or pedestals
			if (!IsPointOnGroundFloor(RandomNavPoint.Location))
			{
				continue;
			}

			if (FVector::DistSquared2D(PathStartPos, RandomNavPoint.Location) < FMath::Square(180.0f))
			{
				continue;
			}

			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, RandomNavPoint.Location, this);
			if (IsGroundNavPathValid(NavPath))
			{
				CurrentPathPoints.Reserve(NavPath->PathPoints.Num());
				for (const FVector& Pt : NavPath->PathPoints)
				{
					CurrentPathPoints.Add(FVector(Pt.X, Pt.Y, GroundFloorZ));
				}
				CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
				return;
			}
		}
	}
}

void AEnemyBatteryThief::FollowCurrentNavPath(float DeltaTime, float SpeedToUse)
{
	if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
	{
		return;
	}

	// Lock ground Z strictly to GroundFloorZ so the spider thief never climbs or flies above ground level
	CurrentGroundZ = GroundFloorZ;
	const float LockedGroundZ = GroundFloorZ + GroundHeightOffset;

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetCorner = CurrentPathPoints[CurrentPathCornerIndex];

	const FVector ToTarget2D(TargetCorner.X - CurrentLocation.X, TargetCorner.Y - CurrentLocation.Y, 0.0f);
	const float Dist2D = ToTarget2D.Size();
	const float StepDist = SpeedToUse * DeltaTime;

	if (Dist2D <= FMath::Max(StepDist, AcceptanceRadius))
	{
		++CurrentPathCornerIndex;
		SetActorLocation(FVector(CurrentLocation.X, CurrentLocation.Y, LockedGroundZ));
		return;
	}

	const FVector MoveDir2D = ToTarget2D / Dist2D;
	const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));

	FVector DesiredNewXY(
		CurrentLocation.X + MoveDir2D.X * StepDist,
		CurrentLocation.Y + MoveDir2D.Y * StepDist,
		LockedGroundZ
	);

	// Sweep at ground level against walls/obstacles so the spider thief can never clip through or go over walls
	if (UWorld* World = GetWorld())
	{
		const float SweepHeightZ = GroundFloorZ + FMath::Max(22.0f, GroundCollisionRadius + 4.0f);
		const FVector SweepStart(CurrentLocation.X, CurrentLocation.Y, SweepHeightZ);
		const FVector SweepEnd(DesiredNewXY.X, DesiredNewXY.Y, SweepHeightZ);

		FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(BatteryThiefWallSweep), false, this);
		if (IsValid(StolenBattery))
		{
			SweepParams.AddIgnoredActor(StolenBattery);
		}

		FHitResult WallHit;
		const FCollisionShape SphereShape = FCollisionShape::MakeSphere(GroundCollisionRadius);
		if (World->SweepSingleByChannel(WallHit, SweepStart, SweepEnd, FQuat::Identity, ECC_WorldStatic, SphereShape, SweepParams))
		{
			AActor* HitActor = WallHit.GetActor();
			const bool bIgnoreHit = HitActor && (HitActor->IsA(AEnemyBatteryThief::StaticClass()) || HitActor->IsA(ABatteryCapsule::StaticClass()));
			if (!bIgnoreHit && WallHit.bBlockingHit)
			{
				// Slide along the ground wall normal instead of going through or over the obstacle
				const FVector SafeStopXY(
					SweepStart.X + (SweepEnd.X - SweepStart.X) * FMath::Max(0.0f, WallHit.Time - 0.02f),
					SweepStart.Y + (SweepEnd.Y - SweepStart.Y) * FMath::Max(0.0f, WallHit.Time - 0.02f),
					LockedGroundZ
				);

				const FVector WallNormal2D = FVector(WallHit.Normal.X, WallHit.Normal.Y, 0.0f).GetSafeNormal();
				const FVector SlideDir2D = FVector::VectorPlaneProject(MoveDir2D, WallNormal2D).GetSafeNormal();
				const float RemainingDist = StepDist * (1.0f - WallHit.Time);

				FVector FinalGroundPos = SafeStopXY;
				if (!SlideDir2D.IsNearlyZero() && RemainingDist > 0.5f)
				{
					const FVector SlideStart(SafeStopXY.X, SafeStopXY.Y, SweepHeightZ);
					const FVector SlideEnd = SlideStart + SlideDir2D * RemainingDist;
					FHitResult SlideHit;
					if (!World->SweepSingleByChannel(SlideHit, SlideStart, SlideEnd, FQuat::Identity, ECC_WorldStatic, SphereShape, SweepParams))
					{
						FinalGroundPos.X = SlideEnd.X;
						FinalGroundPos.Y = SlideEnd.Y;
					}
				}

				SetActorLocation(FVector(FinalGroundPos.X, FinalGroundPos.Y, LockedGroundZ));
				// Force a fresh NavMesh path calculation around the obstacle
				PathRepathTimer = 0.0f;
				return;
			}
		}
	}

	SetActorLocation(FVector(DesiredNewXY.X, DesiredNewXY.Y, LockedGroundZ));
}

void AEnemyBatteryThief::ReturnToPatrol()
{
	TargetLaserCylinder = nullptr;
	CurrentLaserGroundApproachPoint = FVector::ZeroVector;
	RemovalTimerRemaining = 0.0f;
	CurrentState = IsValid(StolenBattery) ? EBatteryThiefState::Carrying : EBatteryThiefState::Patrolling;
	RefreshEnemySettings();

	if (CurrentState == EBatteryThiefState::Carrying)
	{
		TargetDropSpotActor = FindClosestBatteryDropSpot();
		const FVector DestLoc = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		BuildPathToWorldLocation(DestLoc);
	}
	else
	{
		TargetDropSpotActor = nullptr;
		PickNextNavMeshPatrolDestination();
	}
}

void AEnemyBatteryThief::CompleteBatteryRemoval()
{
	if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
	{
		ReturnToPatrol();
		return;
	}

	const int32 SabotagedID = TargetLaserCylinder->CellID;
	ABatteryCapsule* ExtractedBattery = TargetLaserCylinder->RemoveBattery();

	if (IsValid(ExtractedBattery))
	{
		StolenBattery = ExtractedBattery;
		// Mount the stolen battery visibly on top of the Battery Thief's back
		ExtractedBattery->OnSocketed(this, CarriedBatterySocket ? CarriedBatterySocket.Get() : SceneRoot.Get());
		ExtractedBattery->SetActorRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
	}

	TargetLaserCylinder = nullptr;
	CurrentLaserGroundApproachPoint = FVector::ZeroVector;
	RemovalTimerRemaining = 0.0f;

	if (IsValid(StolenBattery))
	{
		CurrentState = EBatteryThiefState::Carrying;
		PathRepathTimer = 0.6f;
		RefreshEnemySettings();

		// Scan the NavMesh for the closest placed BP_BatteryDropSpot actor in the level
		TargetDropSpotActor = FindClosestBatteryDropSpot();
		const FVector DropSpotDestination = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		BuildPathToWorldLocation(DropSpotDestination);

		if (GEngine)
		{
			const FString SpotName = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorNameOrLabel() : TEXT("Fallback Origin");
			GEngine->AddOnScreenDebugMessage(
				3105,
				4.5f,
				FColor::Red,
				FString::Printf(TEXT("[BATTERY SABOTAGED!] Battery Thief removed Battery (ID: %d) and is carrying it to closest Drop Spot (%s)!"), SabotagedID, *SpotName),
				true,
				FVector2D(1.35f, 1.35f)
			);
		}
	}
	else
	{
		CurrentState = EBatteryThiefState::Patrolling;
		TargetDropSpotActor = nullptr;
		RefreshEnemySettings();
		PickNextNavMeshPatrolDestination();
	}
}

void AEnemyBatteryThief::DepositStolenBatteryAtDropOff()
{
	if (IsValid(StolenBattery))
	{
		const int32 DroppedID = StolenBattery->BatteryID;
		const FVector RawDropLocation = IsValid(TargetDropSpotActor)
			? TargetDropSpotActor->GetActorLocation()
			: FVector(GetActorLocation().X, GetActorLocation().Y, GroundFloorZ);
		const FVector FinalDropLocation(RawDropLocation.X, RawDropLocation.Y, GroundFloorZ);

		// Place the battery cleanly on the ground at the closest BP_BatteryDropSpot actor so the player can collect it again
		StolenBattery->OnDropped(FinalDropLocation);
		StolenBattery = nullptr;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				3106,
				4.0f,
				FColor::Yellow,
				FString::Printf(TEXT("[BATTERY DROPPED] Battery Thief dropped Battery (ID: %d) at Drop Spot (X=%.0f, Y=%.0f, Z=%.0f)!"),
					DroppedID, FinalDropLocation.X, FinalDropLocation.Y, FinalDropLocation.Z),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}
	}

	TargetDropSpotActor = nullptr;
	CurrentState = EBatteryThiefState::Patrolling;
	WaitTimer = PatrolPointWaitTime;
	RefreshEnemySettings();
	PickNextNavMeshPatrolDestination();
}

void AEnemyBatteryThief::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	// Always keep the spider thief locked onto the ground surface
	CurrentGroundZ = GroundFloorZ;
	const FVector LockedLoc = GetActorLocation();
	if (!FMath::IsNearlyEqual(LockedLoc.Z, GroundFloorZ + GroundHeightOffset, 0.5f))
	{
		SetActorLocation(FVector(LockedLoc.X, LockedLoc.Y, GroundFloorZ + GroundHeightOffset));
	}

	// ==========================================
	// STATE 1: Patrolling the Ground NavMesh
	// ==========================================
	if (CurrentState == EBatteryThiefState::Patrolling)
	{
		// Scan for any laser gun with a working battery where THIS pawn is the single nearest ground responder
		if (ARedPowerCylinder* WorkingLaser = FindNearestWorkingBatteryLaser())
		{
			FVector GroundStealPoint = FVector::ZeroVector;
			float GroundRouteLen = 0.0f;
			if (ComputeGroundStealApproachPoint(WorkingLaser, GroundStealPoint, GroundRouteLen))
			{
				TargetLaserCylinder = WorkingLaser;
				CurrentLaserGroundApproachPoint = GroundStealPoint;
				CurrentState = EBatteryThiefState::Approaching;
				WaitTimer = 0.0f;
				PathRepathTimer = 0.5f;
				RefreshEnemySettings();
				BuildPathToWorldLocation(CurrentLaserGroundApproachPoint);

				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						3101,
						3.0f,
						FColor::Orange,
						FString::Printf(TEXT("[SABOTEUR ALERT] Nearest Battery Thief detected active Laser (ID: %d) and is routing along the ground to steal its battery!"), TargetLaserCylinder->CellID)
					);
				}
				return;
			}
		}

		if (WaitTimer > 0.0f)
		{
			WaitTimer -= DeltaTime;
			return;
		}

		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			PickNextNavMeshPatrolDestination();
			if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
			{
				return;
			}
		}

		FollowCurrentNavPath(DeltaTime, PatrolSpeed);

		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			WaitTimer = PatrolPointWaitTime;
			PickNextNavMeshPatrolDestination();
		}
		return;
	}

	// ==========================================
	// STATE 2: Carrying Stolen Battery to the Closest BP_BatteryDropSpot
	// ==========================================
	if (CurrentState == EBatteryThiefState::Carrying)
	{
		if (!IsValid(StolenBattery))
		{
			ReturnToPatrol();
			return;
		}

		// Re-verify or find the closest BP_BatteryDropSpot on the ground NavMesh
		if (!IsValid(TargetDropSpotActor))
		{
			TargetDropSpotActor = FindClosestBatteryDropSpot();
		}

		const FVector DropTargetLocation = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		const FVector MyLoc = GetActorLocation();
		const float Dist2DToDropOff = FVector::Dist2D(MyLoc, DropTargetLocation);

		// Once we reach the closest BP_BatteryDropSpot on the ground, drop the battery there and return to patrolling
		if (Dist2DToDropOff <= DropOffAcceptanceRadius)
		{
			DepositStolenBatteryAtDropOff();
			return;
		}

		PathRepathTimer -= DeltaTime;
		if (PathRepathTimer <= 0.0f || !CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			PathRepathTimer = 0.6f;
			TargetDropSpotActor = FindClosestBatteryDropSpot();
			const FVector UpdatedDropLoc = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
			BuildPathToWorldLocation(UpdatedDropLoc);
		}

		FollowCurrentNavPath(DeltaTime, CarrySpeed);

		// If the NavMesh path ended right next to the BP_BatteryDropSpot, deposit the battery
		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex) &&
			FVector::Dist2D(GetActorLocation(), DropTargetLocation) <= (DropOffAcceptanceRadius * 2.5f))
		{
			DepositStolenBatteryAtDropOff();
		}
		return;
	}

	// ==========================================
	// STATE 3: Approaching the working battery laser along the ground level (only the nearest pawn)
	// ==========================================
	if (CurrentState == EBatteryThiefState::Approaching)
	{
		// If the battery was removed or stopped powering the laser, resume patrolling!
		if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
		{
			ReturnToPatrol();
			return;
		}

		// Verify a ground route still exists and that THIS pawn is still the single nearest Battery Thief for this laser
		FVector UpdatedGroundStealPoint = FVector::ZeroVector;
		float UpdatedGroundPathLen = TNumericLimits<float>::Max();
		if (!ComputeGroundStealApproachPoint(TargetLaserCylinder, UpdatedGroundStealPoint, UpdatedGroundPathLen) ||
			!IsNearestThiefForCylinder(TargetLaserCylinder, UpdatedGroundPathLen))
		{
			ReturnToPatrol();
			return;
		}

		CurrentLaserGroundApproachPoint = UpdatedGroundStealPoint;

		const FVector MyLoc = GetActorLocation();
		const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(TargetLaserCylinder);
		const float Dist2DToTriggerGround = FVector::Dist2D(MyLoc, TriggerGroundCenter);
		const float Dist2DToApproachPoint = FVector::Dist2D(MyLoc, CurrentLaserGroundApproachPoint);

		// Check if we have reached the ground-level steal point beneath/beside the RedPowerCylinder's trigger box
		if (Dist2DToTriggerGround <= RemovalInteractionDistance ||
			Dist2DToApproachPoint <= FMath::Max(AcceptanceRadius * 1.5f, 55.0f))
		{
			CurrentState = EBatteryThiefState::Removing;
			RemovalTimerRemaining = BatteryRemovalDuration;
			CurrentPathPoints.Reset();
			RefreshEnemySettings();

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					3102,
					2.0f,
					FColor::Red,
					FString::Printf(TEXT("[BATTERY REMOVAL STARTED] Removing working battery from Laser (ID: %d) from ground level... %.0f seconds remaining!"),
						TargetLaserCylinder->CellID, RemovalTimerRemaining),
					true,
					FVector2D(1.25f, 1.25f)
				);
			}
			return;
		}

		PathRepathTimer -= DeltaTime;
		if (PathRepathTimer <= 0.0f || !CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			PathRepathTimer = 0.6f;
			BuildPathToWorldLocation(CurrentLaserGroundApproachPoint);
		}

		FollowCurrentNavPath(DeltaTime, PatrolSpeed);
		return;
	}

	// ==========================================
	// STATE 4: Removing the working battery from ground level (10-second countdown timer)
	// ==========================================
	if (CurrentState == EBatteryThiefState::Removing)
	{
		// If the player removes the battery before the 10s timer finishes, cancel removal and return to patrol
		if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					3102,
					2.5f,
					FColor::Green,
					TEXT("[SABOTAGE INTERRUPTED] Battery no longer on laser — Battery Thief returning to patrol.")
				);
			}
			ReturnToPatrol();
			return;
		}

		// Face the target laser cylinder on the horizontal ground plane while disconnecting the battery
		const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(TargetLaserCylinder);
		const FVector ToLaser2D(TriggerGroundCenter.X - GetActorLocation().X,
			TriggerGroundCenter.Y - GetActorLocation().Y, 0.0f);
		if (!ToLaser2D.IsNearlyZero())
		{
			const FRotator FaceLaserRot(0.0f, ToLaser2D.Rotation().Yaw, 0.0f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FaceLaserRot, DeltaTime, TurnSpeed));
		}

		// Pulse the warning scanner light during the 10-second removal countdown
		if (ScannerLight)
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 12.0f);
			ScannerLight->SetIntensity(ScannerLightIntensity * (0.4f + 1.1f * Pulse));
		}

		RemovalTimerRemaining -= DeltaTime;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				3102,
				0.15f,
				FColor::Red,
				FString::Printf(TEXT("[SABOTAGING LASER %d] Removing Battery... %.1fs remaining"),
					TargetLaserCylinder->CellID, FMath::Max(0.0f, RemovalTimerRemaining)),
				true,
				FVector2D(1.25f, 1.25f)
			);
		}

		if (RemovalTimerRemaining <= 0.0f)
		{
			CompleteBatteryRemoval();
		}
	}
}

void AEnemyBatteryThief::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
