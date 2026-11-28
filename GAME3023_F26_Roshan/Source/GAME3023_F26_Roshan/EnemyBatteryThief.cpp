#include "EnemyBatteryThief.h"
#include "GAME3023_F26_RoshanGameMode.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "RedPowerCylinder.h"
#include "BatteryCapsule.h"
#include "LiveWireUtils.h"
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
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Constructor: creates the thief movement, scanner light, battery socket, and touch sensor.
AEnemyBatteryThief::AEnemyBatteryThief()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FClassFinder<AActor> DropSpotClassFinder(TEXT("/Game/_Blueprints/LevelAssets/BP_BatteryDropSpot"));
	if (DropSpotClassFinder.Succeeded())
	{
		BatteryDropSpotClass = DropSpotClassFinder.Class;
	}

	CarriedBatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("CarriedBatterySocket"));
	CarriedBatterySocket->SetupAttachment(SceneRoot);
	CarriedBatterySocket->SetRelativeLocation(FVector(0.0f, 0.0f, 42.0f));

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

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = SceneRoot;
	MovementComponent->MaxSpeed = PatrolSpeed;
	MovementComponent->Acceleration = 1200.0f;
	MovementComponent->Deceleration = 2000.0f;
	MovementComponent->NavAgentProps.AgentRadius = 34.0f;
	MovementComponent->NavAgentProps.AgentHeight = 44.0f;

	PlayerTouchTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("PlayerTouchTrigger"));
	PlayerTouchTrigger->SetupAttachment(SceneRoot);
	PlayerTouchTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	PlayerTouchTrigger->InitSphereRadius(52.0f);
	PlayerTouchTrigger->SetCollisionProfileName(TEXT("Trigger"));
	PlayerTouchTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlayerTouchTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PlayerTouchTrigger->OnComponentBeginOverlap.AddDynamic(this, &AEnemyBatteryThief::OnPlayerTouchBeginOverlap);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

// OnConstruction: refreshes thief settings and appearance in the editor.
void AEnemyBatteryThief::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshEnemySettings();
}

#if WITH_EDITOR
// PostEditChangeProperty: refreshes thief settings after an editor property changes.
void AEnemyBatteryThief::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshEnemySettings();
}
#endif

// Updates light intensity and colors from instance properties
void AEnemyBatteryThief::RefreshEnemySettings()
{
	if (ScannerLight)
	{
		ScannerLight->SetIntensity(ScannerLightIntensity);
		const FLinearColor ActiveColor = (CurrentState == EBatteryThiefState::Approaching || CurrentState == EBatteryThiefState::Removing || CurrentState == EBatteryThiefState::FollowingPlayer)
			? AlertLightColor
			: ScannerLightColor;
		ScannerLight->SetLightColor(ActiveColor);
		ScannerLight->SetInnerConeAngle(ScannerOuterConeAngle * 0.6f);
		ScannerLight->SetOuterConeAngle(ScannerOuterConeAngle);
		ScannerLight->SetAttenuationRadius(ScannerLightRange);
	}

	if (MovementComponent)
	{
		float ActiveSpeed = PatrolSpeed;
		if (CurrentState == EBatteryThiefState::FollowingPlayer)
		{
			ActiveSpeed = PatrolSpeed + FollowPlayerSpeedBonus;
		}
		else if (CurrentState == EBatteryThiefState::Carrying)
		{
			ActiveSpeed = CarrySpeed;
		}
		MovementComponent->MaxSpeed = ActiveSpeed;
	}
}

// Disables mesh NavMesh carving, sets health, and starts patrol loop
void AEnemyBatteryThief::BeginPlay()
{
	Super::BeginPlay();

	// Ensure Blueprint-added visual meshes don't carve the NavMesh or block other thieves/pawns
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* MeshComp : MeshComps)
	{
		if (MeshComp)
		{
			MeshComp->SetCanEverAffectNavigation(false);
			MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}

	CurrentHealth = MaxHealth;
	bIsDead = false;

	if (FMath::IsNearlyEqual(PatrolSpeed, 260.0f))
	{
		PatrolSpeed = 300.0f;
	}
	if (FMath::IsNearlyEqual(CarrySpeed, 160.0f))
	{
		CarrySpeed = 300.0f;
	}

	RefreshEnemySettings();

	if (bSnapToSpawnLocationOnStart)
	{
		SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	}

	const FVector StartPos = GetActorLocation();
	InitialSpawnZ = SampleGroundNavMeshZ(StartPos);
	CurrentGroundZ = InitialSpawnZ;
	bHasValidGroundZ = true;
	PatrolOrigin = FVector(StartPos.X, StartPos.Y, InitialSpawnZ);

	SetActorLocation(FVector(StartPos.X, StartPos.Y, InitialSpawnZ + GroundHeightOffset));

	CurrentState = EBatteryThiefState::Patrolling;
	PickNextNavMeshPatrolDestination();
}

// EndPlay: clears timers and removes the thief from active gameplay.
void AEnemyBatteryThief::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(StolenBattery))
	{
		const FVector SafeDropPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);
		StolenBattery->OnDropped(SafeDropPos);
		StolenBattery = nullptr;
	}

	if (!bIsDead && EndPlayReason == EEndPlayReason::Destroyed)
	{
		bIsDead = true;
	}

	Super::EndPlay(EndPlayReason);
}

// TakeDamage: reduces thief health and calls the death behavior when health reaches zero.
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

// ApplyDamage: updates health and processes damage
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

// HandleDeath: starts the thief death sequence and broadcasts the killed event.
void AEnemyBatteryThief::HandleDeath()
{
	KillBatteryThief();
}

// ApplyHealing: handles apply healing
void AEnemyBatteryThief::ApplyHealing(float Healing, AActor* Healer)
{
	if (bIsDead || Healing <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Healing, 0.0f, MaxHealth);
}

// NotifyDanger: handles notify danger
void AEnemyBatteryThief::NotifyDanger(const FVector& DangerLocation, AActor* DangerSource)
{
}

// KillBatteryThief: manages battery attachment and state
void AEnemyBatteryThief::KillBatteryThief()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	if (IsValid(StolenBattery))
	{
		const FVector SafeDropPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);
		StolenBattery->OnDropped(SafeDropPos);
		StolenBattery = nullptr;
	}

	TargetLaserCylinder = nullptr;
	TargetDropSpotActor = nullptr;
	TargetPlayerPawn = nullptr;
	bIsFollowingPlayer = false;

	Destroy();
}

// KillPermanently: destroys the thief immediately after a fatal effect or laser hit.
void AEnemyBatteryThief::KillPermanently()
{
	if (bIsDead)
	{
		return;
	}

	KillBatteryThief();
}

// IsPointOnWalkableSurface: handles is point on walkable surface
bool AEnemyBatteryThief::IsPointOnWalkableSurface(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedNav;
		return NavSys->ProjectPointToNavigation(Point, ProjectedNav, FVector(150.0f, 150.0f, 350.0f));
	}

	return true;
}

// IsGroundNavPathValid: updates movement pathfinding and waypoint navigation
bool AEnemyBatteryThief::IsGroundNavPathValid(const UNavigationPath* NavPath) const
{
	if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial() || NavPath->PathPoints.Num() < 2)
	{
		return false;
	}

	for (int32 i = 0; i < NavPath->PathPoints.Num() - 1; ++i)
	{
		const FVector& A = NavPath->PathPoints[i];
		const FVector& B = NavPath->PathPoints[i + 1];

		const float Dist2D = FVector::Dist2D(A, B);
		const float DeltaZ = FMath::Abs(B.Z - A.Z);

		if (Dist2D < 10.0f && DeltaZ > 80.0f)
		{
			return false;
		}

		if (Dist2D > 1.0f && DeltaZ > 60.0f && (DeltaZ / Dist2D) > 2.5f)
		{
			return false;
		}
	}

	return true;
}

// GetCylinderTriggerGroundCenter: handles get cylinder trigger ground center
FVector AEnemyBatteryThief::GetCylinderTriggerGroundCenter(const ARedPowerCylinder* Cylinder) const
{
	if (!IsValid(Cylinder))
	{
		return GetActorLocation();
	}

	UWorld* World = GetWorld();
	const FVector CylinderBase = Cylinder->GetActorLocation();

	if (World)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation ProjectedNav;
			if (NavSys->ProjectPointToNavigation(CylinderBase, ProjectedNav, FVector(200.0f, 200.0f, 400.0f)))
			{
				return ProjectedNav.Location;
			}
		}

		FHitResult Hit;
		const FVector TraceStart = CylinderBase + FVector(0.0f, 0.0f, 100.0f);
		const FVector TraceEnd   = CylinderBase - FVector(0.0f, 0.0f, 400.0f);
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(CylinderFloorTrace), false, Cylinder);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams))
		{
			if (Hit.Normal.Z >= 0.5f)
			{
				return Hit.ImpactPoint;
			}
		}
	}

	return CylinderBase;
}

// ComputeGroundStealApproachPoint: handles compute ground steal approach point
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

	const FVector CurrentLocation = GetActorLocation();
	FVector PathStartPos = CurrentLocation;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(CurrentLocation, ProjectedStart, FVector(200.0f, 200.0f, 350.0f)))
	{
		PathStartPos = ProjectedStart.Location;
	}

	const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(Cylinder);
	const FVector ToThief2D = (PathStartPos - TriggerGroundCenter).GetSafeNormal2D();
	const float BaseAngleRad = ToThief2D.IsNearlyZero() ? 0.0f : FMath::Atan2(ToThief2D.Y, ToThief2D.X);

	TArray<FVector, TInlineAllocator<48>> CandidateGroundPoints;
	CandidateGroundPoints.Add(TriggerGroundCenter);

	const float RingRadii[] = { 80.0f, 140.0f, 200.0f, 260.0f, 320.0f };
	const float AngleOffsetsDeg[] = { 0.0f, 45.0f, -45.0f, 90.0f, -90.0f, 135.0f, -135.0f, 180.0f };

	for (float Radius : RingRadii)
	{
		for (float OffsetDeg : AngleOffsetsDeg)
		{
			const float AngleRad = BaseAngleRad + FMath::DegreesToRadians(OffsetDeg);
			CandidateGroundPoints.Add(FVector(
				TriggerGroundCenter.X + FMath::Cos(AngleRad) * Radius,
				TriggerGroundCenter.Y + FMath::Sin(AngleRad) * Radius,
				TriggerGroundCenter.Z
			));
		}
	}

	const float MaxAllowedReach2D = FMath::Max(RemovalInteractionDistance + 120.0f, 350.0f);
	bool bFoundValidGroundRoute = false;

	for (const FVector& Candidate : CandidateGroundPoints)
	{
		FNavLocation ProjectedTarget;
		if (!NavSys->ProjectPointToNavigation(Candidate, ProjectedTarget, FVector(150.0f, 150.0f, 350.0f)))
		{
			continue;
		}

		if (!IsPointOnWalkableSurface(ProjectedTarget.Location))
		{
			continue;
		}

		if (FVector::Dist2D(ProjectedTarget.Location, TriggerGroundCenter) > MaxAllowedReach2D ||
			FMath::Abs(ProjectedTarget.Location.Z - TriggerGroundCenter.Z) > 150.0f)
		{
			continue;
		}

		const float Dist3DFromStart = FVector::Dist(PathStartPos, ProjectedTarget.Location);
		if (Dist3DFromStart <= FMath::Max(AcceptanceRadius, 50.0f))
		{
			OutGroundPoint = ProjectedTarget.Location;
			OutNavPathLength = Dist3DFromStart;
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
				OutGroundPoint = ProjectedTarget.Location;
				bFoundValidGroundRoute = true;
			}
		}
	}

	return bFoundValidGroundRoute;
}

// IsNearestThiefForCylinder: battery thief AI behavior
bool AEnemyBatteryThief::IsNearestThiefForCylinder(const ARedPowerCylinder* Cylinder, float MyGroundPathLength) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Cylinder))
	{
		return false;
	}

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

		if (Other->CurrentState == EBatteryThiefState::Removing && Other->TargetLaserCylinder == Cylinder)
		{
			return false;
		}

		if (Other->CurrentState == EBatteryThiefState::Carrying || Other->CurrentState == EBatteryThiefState::FollowingPlayer)
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

// DoesLaserHaveWorkingBattery: manages battery attachment and state
bool AEnemyBatteryThief::DoesLaserHaveWorkingBattery(const ARedPowerCylinder* Cylinder) const
{
	if (!IsValid(Cylinder))
	{
		return false;
	}

	return Cylinder->bHasBattery &&
		Cylinder->bIsPowered &&
		IsValid(Cylinder->InsertedBattery) &&
		(Cylinder->InsertedBattery->BatteryShape == Cylinder->RequiredShape);
}

// FindNearestWorkingBatteryLaser: manages battery attachment and state
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

		FVector CandidateGroundPoint = FVector::ZeroVector;
		float CandidatePathLength = TNumericLimits<float>::Max();
		if (!ComputeGroundStealApproachPoint(Cylinder, CandidateGroundPoint, CandidatePathLength))
		{
			continue;
		}

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

// FindNearestDropPad: finds the closest location where the thief can deposit a battery.
AActor* AEnemyBatteryThief::FindClosestBatteryDropSpot() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> CandidateDropSpots;

	if (BatteryDropSpotClass)
	{
		UGameplayStatics::GetAllActorsOfClass(World, BatteryDropSpotClass, CandidateDropSpots);
	}

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
	const FVector CurrentLocation = GetActorLocation();

	FVector PathStartPos = CurrentLocation;
	if (NavSys)
	{
		FNavLocation ProjectedStart;
		if (NavSys->ProjectPointToNavigation(CurrentLocation, ProjectedStart, FVector(200.0f, 200.0f, 250.0f)))
		{
			PathStartPos = ProjectedStart.Location;
		}
	}

	AActor* ClosestDropSpot = nullptr;
	float ShortestNavCost = TNumericLimits<float>::Max();

	for (AActor* Spot : CandidateDropSpots)
	{
		if (!IsValid(Spot))
		{
			continue;
		}

		const FVector SpotLoc = Spot->GetActorLocation();
		float CandidateDistanceScore = FVector::Dist(CurrentLocation, SpotLoc);

		if (NavSys)
		{
			FNavLocation ProjectedNav;
			if (NavSys->ProjectPointToNavigation(SpotLoc, ProjectedNav, FVector(350.0f, 350.0f, 350.0f)))
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

// Projects location onto NavMesh floor to prevent floating or sinking
float AEnemyBatteryThief::SampleGroundNavMeshZ(const FVector& WorldPos) const
{
	FVector ProjectedNav;
	if (LiveWireUtils::ProjectToNavMesh(GetWorld(), WorldPos, FVector(100.0f, 100.0f, 350.0f), ProjectedNav))
	{
		return ProjectedNav.Z;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return WorldPos.Z;
	}

	FHitResult Hit;
	const FVector TraceStart = WorldPos + FVector(0.0f, 0.0f, 80.0f);
	const FVector TraceEnd   = WorldPos - FVector(0.0f, 0.0f, 350.0f);
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(BatteryThiefGroundSample), false, this);
	if (IsValid(StolenBattery))
	{
		TraceParams.AddIgnoredActor(StolenBattery);
	}
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams) && Hit.Normal.Z >= 0.5f)
	{
		return Hit.ImpactPoint.Z;
	}

	return WorldPos.Z;
}

// BuildPathToWorldLocation: handles build path to world location
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

	const FVector CurrentLocation = GetActorLocation();
	FVector PathStartPos = CurrentLocation;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(CurrentLocation, ProjectedStart, FVector(200.0f, 200.0f, 350.0f)))
	{
		PathStartPos = ProjectedStart.Location;
	}

	FNavLocation ProjectedTarget;
	if (!NavSys->ProjectPointToNavigation(DestinationWorldPos, ProjectedTarget, FVector(250.0f, 250.0f, 400.0f)))
	{
		return false;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, ProjectedTarget.Location, this);
	if (IsGroundNavPathValid(NavPath))
	{
		CurrentPathPoints = NavPath->PathPoints;
		CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
		return true;
	}

	return false;
}

// Picks random reachable spot on NavMesh for ambient patrol
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

	const FVector CurrentLocation = GetActorLocation();
	FVector PathStartPos = CurrentLocation;
	FNavLocation ProjectedStart;
	if (NavSys->ProjectPointToNavigation(CurrentLocation, ProjectedStart, FVector(200.0f, 200.0f, 350.0f)))
	{
		PathStartPos = ProjectedStart.Location;
	}

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

	TArray<FVector, TInlineAllocator<16>> AnchorPoints;
	AnchorPoints.Add(PathStartPos);

	TArray<AActor*> ArenaCylinders;
	UGameplayStatics::GetAllActorsOfClass(World, ARedPowerCylinder::StaticClass(), ArenaCylinders);
	for (AActor* Act : ArenaCylinders)
	{
		if (IsValid(Act))
		{
			AnchorPoints.Add(Act->GetActorLocation());
		}
	}

	if (BatteryDropSpotClass)
	{
		TArray<AActor*> DropSpots;
		UGameplayStatics::GetAllActorsOfClass(World, BatteryDropSpotClass, DropSpots);
		for (AActor* Act : DropSpots)
		{
			if (IsValid(Act))
			{
				AnchorPoints.Add(Act->GetActorLocation());
			}
		}
	}

	for (int32 Attempt = 0; Attempt < 30; ++Attempt)
	{
		FNavLocation RandomNavPoint;
		bool bFoundPoint = false;

		const int32 Strategy = Attempt % 3;
		if (Strategy == 0 && AnchorPoints.Num() > 0)
		{
			const int32 RandomIdx = FMath::RandRange(0, AnchorPoints.Num() - 1);
			const FVector& Anchor = AnchorPoints[RandomIdx];
			bFoundPoint = NavSys->GetRandomReachablePointInRadius(Anchor, 2000.0f, RandomNavPoint);
		}
		else if (Strategy == 1)
		{
			const float WanderRadius = FMath::Max(PatrolRadius, 6000.0f);
			bFoundPoint = NavSys->GetRandomReachablePointInRadius(PathStartPos, WanderRadius, RandomNavPoint);
		}
		else
		{
			bFoundPoint = NavSys->GetRandomPoint(RandomNavPoint);
		}

		if (bFoundPoint)
		{
			if (FVector::DistSquared2D(PathStartPos, RandomNavPoint.Location) < FMath::Square(220.0f))
			{
				continue;
			}

			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, RandomNavPoint.Location, this);
			if (IsGroundNavPathValid(NavPath))
			{
				CurrentPathPoints = NavPath->PathPoints;
				CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
				return;
			}
		}
	}
}

// TickMoving: advances the thief along its current navigation path.
void AEnemyBatteryThief::FollowCurrentNavPath(float DeltaTime, float SpeedToUse)
{
	if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetCorner = CurrentPathPoints[CurrentPathCornerIndex];
	const FVector ToTarget2D(TargetCorner.X - CurrentLocation.X, TargetCorner.Y - CurrentLocation.Y, 0.0f);
	const float Dist2D = ToTarget2D.Size();
	const float StepDist = SpeedToUse * DeltaTime;
	const bool bHasNextCorner = CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex + 1);
	const float CornerAcceptance = bHasNextCorner ? FMath::Max(AcceptanceRadius * 1.6f, 55.0f) : AcceptanceRadius;

	if (Dist2D <= FMath::Max(StepDist, CornerAcceptance))
	{
		++CurrentPathCornerIndex;
		PinToGroundSurface();
		return;
	}

	const FVector MoveDir2D = ToTarget2D / Dist2D;
	const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));

	const float SampledSurfaceZ = SampleGroundNavMeshZ(FVector(
		CurrentLocation.X + MoveDir2D.X * StepDist,
		CurrentLocation.Y + MoveDir2D.Y * StepDist,
		TargetCorner.Z));
	CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledSurfaceZ, DeltaTime, 20.0f);
	const float TargetZ = CurrentGroundZ + GroundHeightOffset;

	SetActorLocation(FVector(
		CurrentLocation.X + MoveDir2D.X * StepDist,
		CurrentLocation.Y + MoveDir2D.Y * StepDist,
		TargetZ));
}

// ReturnToPatrol: clears the current task and sends the thief back to patrol.
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

// CompleteBatteryRemoval: manages battery attachment and state
void AEnemyBatteryThief::CompleteBatteryRemoval()
{
	if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
	{
		ReturnToPatrol();
		return;
	}

	ABatteryCapsule* ExtractedBattery = TargetLaserCylinder->RemoveBattery();

	if (IsValid(ExtractedBattery))
	{
		StolenBattery = ExtractedBattery;
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

		TargetDropSpotActor = FindClosestBatteryDropSpot();
		const FVector DropSpotDestination = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		BuildPathToWorldLocation(DropSpotDestination);
	}
	else
	{
		CurrentState = EBatteryThiefState::Patrolling;
		TargetDropSpotActor = nullptr;
		RefreshEnemySettings();
		PickNextNavMeshPatrolDestination();
	}
}

// DepositStolenBatteryAtDropOff: manages battery attachment and state
void AEnemyBatteryThief::DepositStolenBatteryAtDropOff()
{
	if (IsValid(StolenBattery))
	{
		const FVector RawDropLocation = IsValid(TargetDropSpotActor)
			? TargetDropSpotActor->GetActorLocation()
			: GetActorLocation();
		const float DropZ = SampleGroundNavMeshZ(RawDropLocation);
		const FVector FinalDropLocation(RawDropLocation.X, RawDropLocation.Y, DropZ);

		StolenBattery->OnDropped(FinalDropLocation);
		StolenBattery = nullptr;
	}

	TargetDropSpotActor = nullptr;
	CurrentState = EBatteryThiefState::Patrolling;
	WaitTimer = PatrolPointWaitTime;
	RefreshEnemySettings();
	PickNextNavMeshPatrolDestination();
}

// PinToGroundSurface: handles pin to ground surface
void AEnemyBatteryThief::PinToGroundSurface()
{
	const FVector CurrentLoc = GetActorLocation();
	const float SurfaceZ = SampleGroundNavMeshZ(CurrentLoc);
	CurrentGroundZ = SurfaceZ;
	SetActorLocation(FVector(CurrentLoc.X, CurrentLoc.Y, SurfaceZ + GroundHeightOffset));
}

// CheckPlayerProximityTouch: handles check player proximity touch
void AEnemyBatteryThief::CheckPlayerProximityTouch()
{
	if (bIsFollowingPlayer || bIsDead)
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(PlayerPawn);
	if (!PlayerChar || PlayerChar->IsDead())
	{
		return;
	}

	const float Dist2D = FVector::Dist2D(GetActorLocation(), PlayerPawn->GetActorLocation());
	const float DistZ = FMath::Abs(GetActorLocation().Z - PlayerPawn->GetActorLocation().Z);
	if (Dist2D <= 75.0f && DistZ <= 110.0f)
	{
		StartFollowingPlayer(PlayerPawn);
	}
}

// Tick: runs the thief behavior for its current patrol, removal, carrying, or follow state.
void AEnemyBatteryThief::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	if (CurrentState == EBatteryThiefState::Removing || WaitTimer > 0.0f)
	{
		PinToGroundSurface();
	}

	CheckPlayerProximityTouch();

	switch (CurrentState)
	{
	case EBatteryThiefState::Patrolling:
		TickPatrolling(DeltaTime);
		break;
	case EBatteryThiefState::Carrying:
		TickCarrying(DeltaTime);
		break;
	case EBatteryThiefState::Approaching:
		TickApproaching(DeltaTime);
		break;
	case EBatteryThiefState::Removing:
		TickRemoving(DeltaTime);
		break;
	case EBatteryThiefState::FollowingPlayer:
		TickFollowingPlayer(DeltaTime);
		break;
	default:
		break;
	}
}

// TickPatrolling: updates movement pathfinding and waypoint navigation
void AEnemyBatteryThief::TickPatrolling(float DeltaTime)
{
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
}

// TickCarrying: handles tick carrying
void AEnemyBatteryThief::TickCarrying(float DeltaTime)
{
	if (!IsValid(StolenBattery))
	{
		ReturnToPatrol();
		return;
	}

	if (!IsValid(TargetDropSpotActor))
	{
		TargetDropSpotActor = FindClosestBatteryDropSpot();
	}

	const FVector DropTargetLocation = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
	if (FVector::Dist2D(GetActorLocation(), DropTargetLocation) <= DropOffAcceptanceRadius)
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

	if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex) &&
		FVector::Dist2D(GetActorLocation(), DropTargetLocation) <= (DropOffAcceptanceRadius * 2.5f))
	{
		DepositStolenBatteryAtDropOff();
	}
}

// TickApproaching: handles tick approaching
void AEnemyBatteryThief::TickApproaching(float DeltaTime)
{
	if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
	{
		ReturnToPatrol();
		return;
	}

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
	const float DistZToTriggerGround = FMath::Abs(MyLoc.Z - TriggerGroundCenter.Z);
	const float Dist3DToApproachPoint = FVector::Dist(MyLoc, CurrentLaserGroundApproachPoint);

	if ((Dist2DToTriggerGround <= RemovalInteractionDistance && DistZToTriggerGround <= 160.0f) ||
		Dist3DToApproachPoint <= FMath::Max(AcceptanceRadius * 1.5f, 65.0f))
	{
		CurrentState = EBatteryThiefState::Removing;
		RemovalTimerRemaining = BatteryRemovalDuration;
		CurrentPathPoints.Reset();
		RefreshEnemySettings();
		return;
	}

	PathRepathTimer -= DeltaTime;
	if (PathRepathTimer <= 0.0f || !CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
	{
		PathRepathTimer = 0.6f;
		BuildPathToWorldLocation(CurrentLaserGroundApproachPoint);
	}

	FollowCurrentNavPath(DeltaTime, PatrolSpeed);
}

// TickRemoving: handles tick removing
void AEnemyBatteryThief::TickRemoving(float DeltaTime)
{
	if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
	{
		ReturnToPatrol();
		return;
	}

	const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(TargetLaserCylinder);
	const FVector ToLaser2D(TriggerGroundCenter.X - GetActorLocation().X, TriggerGroundCenter.Y - GetActorLocation().Y, 0.0f);
	if (!ToLaser2D.IsNearlyZero())
	{
		const FRotator FaceLaserRot(0.0f, ToLaser2D.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), FaceLaserRot, DeltaTime, TurnSpeed));
	}

	PinToGroundSurface();

	if (ScannerLight && GetWorld())
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 12.0f);
		ScannerLight->SetIntensity(ScannerLightIntensity * (0.4f + 1.1f * Pulse));
	}

	RemovalTimerRemaining -= DeltaTime;

	if (RemovalTimerRemaining <= 0.0f)
	{
		CompleteBatteryRemoval();
	}
}

// TickFollowingPlayer: handles tick following player
void AEnemyBatteryThief::TickFollowingPlayer(float DeltaTime)
{
	const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetPlayerPawn);
	if (!IsValid(TargetPlayerPawn) || (PlayerChar && PlayerChar->IsDead()))
	{
		StopFollowingPlayerAndResumePatrol();
		return;
	}

	const FVector PlayerLoc = TargetPlayerPawn->GetActorLocation();
	const float Dist3DToPlayer = FVector::Dist(GetActorLocation(), PlayerLoc);

	PathRepathTimer -= DeltaTime;
	if (PathRepathTimer <= 0.0f || !CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
	{
		PathRepathTimer = 0.35f;
		BuildPathToWorldLocation(PlayerLoc);
	}

	const float FollowSpeed = PatrolSpeed + FollowPlayerSpeedBonus;
	if (Dist3DToPlayer <= 70.0f)
	{
		const FVector ToPlayer2D(PlayerLoc.X - GetActorLocation().X, PlayerLoc.Y - GetActorLocation().Y, 0.0f);
		if (!ToPlayer2D.IsNearlyZero())
		{
			const FRotator FaceRot(0.0f, ToPlayer2D.Rotation().Yaw, 0.0f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), FaceRot, DeltaTime, TurnSpeed));
		}
	}
	else
	{
		FollowCurrentNavPath(DeltaTime, FollowSpeed);
	}
}

// StartFollowingPlayer: handles start following player
void AEnemyBatteryThief::StartFollowingPlayer(APawn* InPlayerPawn)
{
	if (!IsValid(InPlayerPawn) || bIsDead)
	{
		return;
	}

	if (bIsFollowingPlayer && TargetPlayerPawn == InPlayerPawn)
	{
		return;
	}

	TargetPlayerPawn = InPlayerPawn;
	bIsFollowingPlayer = true;
	CurrentState = EBatteryThiefState::FollowingPlayer;

	TargetLaserCylinder = nullptr;
	RemovalTimerRemaining = 0.0f;
	CurrentLaserGroundApproachPoint = FVector::ZeroVector;

	CurrentPathPoints.Reset();
	CurrentPathCornerIndex = INDEX_NONE;
	PathRepathTimer = 0.0f;

	RefreshEnemySettings();

	const float FollowSpeed = PatrolSpeed + FollowPlayerSpeedBonus;
	UE_LOG(LogTemp, Warning, TEXT("[AEnemyBatteryThief] Touched by player '%s'! Following player at speed %.1f (Base %.1f + Bonus %.1f)."),
		*GetNameSafe(InPlayerPawn), FollowSpeed, PatrolSpeed, FollowPlayerSpeedBonus);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			3108,
			3.5f,
			FColor::Orange,
			FString::Printf(TEXT("[BATTERY THIEF ALERT] Touched by player! Following player at Speed %.0f (+%.0f)!"),
				FollowSpeed, FollowPlayerSpeedBonus),
			true,
			FVector2D(1.3f, 1.3f)
		);
	}
}

// StopFollowingPlayerAndResumePatrol: updates movement pathfinding and waypoint navigation
void AEnemyBatteryThief::StopFollowingPlayerAndResumePatrol()
{
	bIsFollowingPlayer = false;
	TargetPlayerPawn = nullptr;
	CurrentPathPoints.Reset();
	CurrentPathCornerIndex = INDEX_NONE;
	ReturnToPatrol();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			3109,
			2.5f,
			FColor::Green,
			TEXT("[BATTERY THIEF] Stopped following player — resuming ground NavMesh patrol."),
			true,
			FVector2D(1.2f, 1.2f)
		);
	}
}

// OnPlayerTouchBeginOverlap: trigger zone overlap event
void AEnemyBatteryThief::OnPlayerTouchBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDead || bIsFollowingPlayer || !IsValid(OtherActor))
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == PlayerPawn || OtherActor->IsA<AGAME3023_F26_RoshanCharacter>())
	{
		if (AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
		{
			if (PlayerChar->IsDead())
			{
				return;
			}
		}

		StartFollowingPlayer(Cast<APawn>(OtherActor));
	}
}

// Unused input binding hook for enemy pawn
void AEnemyBatteryThief::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
