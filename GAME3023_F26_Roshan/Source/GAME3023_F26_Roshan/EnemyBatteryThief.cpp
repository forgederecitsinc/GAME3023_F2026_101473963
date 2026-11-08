// roshan - 101473963 - game3023 game engines 3
// ground battery saboteur / battery thief enemy implementation
// spawns at (X=-878.0, Y=-3679.0, Z=10.0), patrols the ground NavMesh, ignores loose or mismatched
// batteries, detects ONLY batteries that work with a laser (bIsPowered && bHasBattery), navigates to
// the nearest active laser battery, runs a 10-second removal timer to detach the battery, scans the NavMesh
// for the closest placed BP_BatteryDropSpot actor in the level, carries the battery there on its back, drops it, and returns to patrol

#include "EnemyBatteryThief.h"
#include "RedPowerCylinder.h"
#include "BatteryCapsule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
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

	// Low-profile ground crawler mesh (can be overridden or hidden in BP_EnemyBatteryThief)
	ThiefMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThiefMesh"));
	ThiefMesh->SetupAttachment(SceneRoot);
	ThiefMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	ThiefMesh->SetRelativeScale3D(BodyMeshScale);
	ThiefMesh->SetCollisionProfileName(TEXT("NoCollision"));

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
	PatrolOrigin = InHomeOrigin;
}

void AEnemyBatteryThief::BeginPlay()
{
	Super::BeginPlay();

	RefreshEnemySettings();

	// If configured to snap to the target spawn coordinates (X=-878, Y=-3679, Z=10)
	if (!bIsSpawnedChildUnit && bSnapToSpawnLocationOnStart)
	{
		SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	}

	const FVector StartPos = GetActorLocation();
	CurrentGroundZ = SampleGroundNavMeshZ(StartPos);
	bHasValidGroundZ = true;

	if (!bIsSpawnedChildUnit)
	{
		// Use the actor's placed/spawned location (or SpawnLocation) as the NavMesh patrol origin
		PatrolOrigin = FVector(StartPos.X, StartPos.Y, CurrentGroundZ);
	}

	// Snap cleanly onto the ground NavMesh surface
	SetActorLocation(FVector(StartPos.X, StartPos.Y, CurrentGroundZ + GroundHeightOffset));

	// Start patrolling the NavMesh right away
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
	Super::EndPlay(EndPlayReason);
}

AEnemyBatteryThief* AEnemyBatteryThief::SpawnThiefAtTargetLocation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (TotalSpawnedCount >= MaxEnemiesToSpawn)
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		return nullptr;
	}

	TSubclassOf<AEnemyBatteryThief> ClassToSpawn = EnemyClassToSpawn ? *EnemyClassToSpawn : GetClass();
	const float GroundZAtSpawn = SampleGroundNavMeshZ(SpawnLocation);
	const FVector FinalSpawnPos(SpawnLocation.X, SpawnLocation.Y, GroundZAtSpawn + GroundHeightOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyBatteryThief* NewThief = World->SpawnActor<AEnemyBatteryThief>(ClassToSpawn, FinalSpawnPos, SpawnRotation, SpawnParams);
	if (NewThief)
	{
		++TotalSpawnedCount;
		NewThief->PatrolSpeed = PatrolSpeed;
		NewThief->CarrySpeed = CarrySpeed;
		NewThief->TurnSpeed = TurnSpeed;
		NewThief->GroundHeightOffset = GroundHeightOffset;
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

		if (TotalSpawnedCount >= MaxEnemiesToSpawn)
		{
			World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		}
	}

	return NewThief;
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
	float BestDistSq = MaxDistSq;

	for (AActor* Actor : FoundCylinders)
	{
		ARedPowerCylinder* Cylinder = Cast<ARedPowerCylinder>(Actor);
		if (!DoesLaserHaveWorkingBattery(Cylinder))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(MyLoc, Cylinder->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
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
	const FVector MyGroundPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);

	FVector PathStartPos = MyGroundPos;
	if (NavSys)
	{
		FNavLocation ProjectedStart;
		if (NavSys->ProjectPointToNavigation(MyGroundPos, ProjectedStart, FVector(500.0f, 500.0f, 500.0f)))
		{
			PathStartPos = ProjectedStart.Location;
		}
	}

	AActor* ClosestDropSpot = nullptr;
	float ShortestNavCost = TNumericLimits<float>::Max();

	// Evaluate every BP_BatteryDropSpot on the NavMesh and pick the one with the shortest NavMesh path length
	for (AActor* Spot : CandidateDropSpots)
	{
		if (!IsValid(Spot))
		{
			continue;
		}

		const FVector SpotLoc = Spot->GetActorLocation();
		float CandidateDistanceScore = FVector::Dist2D(MyGroundPos, SpotLoc);

		if (NavSys)
		{
			FVector ProjectedSpotLoc = SpotLoc;
			FNavLocation ProjectedNav;
			if (NavSys->ProjectPointToNavigation(SpotLoc, ProjectedNav, FVector(500.0f, 500.0f, 500.0f)))
			{
				ProjectedSpotLoc = ProjectedNav.Location;
			}

			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, ProjectedSpotLoc, const_cast<AEnemyBatteryThief*>(this));
			if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 1)
			{
				CandidateDistanceScore = NavPath->GetPathLength();
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
	UWorld* World = GetWorld();
	if (!World)
	{
		return WorldPos.Z;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedNav;
		if (NavSys->ProjectPointToNavigation(WorldPos, ProjectedNav, FVector(250.0f, 250.0f, 500.0f)))
		{
			return ProjectedNav.Location.Z;
		}
	}

	// Downward ground line trace fallback
	FHitResult Hit;
	const FVector TraceStart = WorldPos + FVector(0.0f, 0.0f, 200.0f);
	const FVector TraceEnd = WorldPos - FVector(0.0f, 0.0f, 500.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BatteryThiefGroundTrace), false, this);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.ImpactPoint.Z;
	}

	return bHasValidGroundZ ? CurrentGroundZ : WorldPos.Z;
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
	const FVector CurrentGroundPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);

	if (NavSys)
	{
		FVector PathStartPos = CurrentGroundPos;
		FNavLocation ProjectedStart;
		if (NavSys->ProjectPointToNavigation(CurrentGroundPos, ProjectedStart, FVector(500.0f, 500.0f, 500.0f)))
		{
			PathStartPos = ProjectedStart.Location;
		}

		// Project the target destination onto the nearest walkable NavMesh point
		FVector PathTargetPos = DestinationWorldPos;
		FNavLocation ProjectedTarget;
		if (NavSys->ProjectPointToNavigation(DestinationWorldPos, ProjectedTarget, FVector(450.0f, 450.0f, 500.0f)))
		{
			PathTargetPos = ProjectedTarget.Location;
		}

		UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, PathTargetPos, this);
		if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
		{
			CurrentPathPoints = NavPath->PathPoints;
			CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
			return true;
		}
	}

	// Fallback direct ground point if NavMesh projection wasn't available
	CurrentPathPoints.Add(DestinationWorldPos);
	CurrentPathCornerIndex = 0;
	return true;
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
	const FVector CurrentGroundPos(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);

	FVector PathStartPos = CurrentGroundPos;
	if (NavSys)
	{
		FNavLocation ProjectedStart;
		if (NavSys->ProjectPointToNavigation(CurrentGroundPos, ProjectedStart, FVector(450.0f, 450.0f, 500.0f)))
		{
			PathStartPos = ProjectedStart.Location;
		}
	}

	// 1. If custom patrol waypoints are assigned on the instance, cycle through them
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

	// 2. Otherwise, pick a random reachable ground point on the NavMesh within PatrolRadius
	if (NavSys)
	{
		FVector SearchOrigin = PathStartPos;
		FNavLocation ProjectedOrigin;
		if (NavSys->ProjectPointToNavigation(PatrolOrigin, ProjectedOrigin, FVector(500.0f, 500.0f, 500.0f)))
		{
			SearchOrigin = ProjectedOrigin.Location;
		}

		for (int32 Attempt = 0; Attempt < 12; ++Attempt)
		{
			FNavLocation RandomNavPoint;
			if (NavSys->GetRandomReachablePointInRadius(SearchOrigin, PatrolRadius, RandomNavPoint))
			{
				if (FVector::DistSquared2D(PathStartPos, RandomNavPoint.Location) < FMath::Square(200.0f))
				{
					continue;
				}

				UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, RandomNavPoint.Location, this);
				if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
				{
					CurrentPathPoints = NavPath->PathPoints;
					CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
					return;
				}
			}
		}

		FNavLocation FallbackNavPoint;
		if (NavSys->GetRandomPointInNavigableRadius(PathStartPos, PatrolRadius, FallbackNavPoint))
		{
			CurrentPathPoints.Add(FallbackNavPoint.Location);
			CurrentPathCornerIndex = 0;
		}
	}
}

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

	FVector NewLocation = CurrentLocation;

	if (Dist2D <= FMath::Max(StepDist, AcceptanceRadius))
	{
		++CurrentPathCornerIndex;
	}
	else
	{
		const FVector MoveDir2D = ToTarget2D / Dist2D;
		NewLocation.X += MoveDir2D.X * StepDist;
		NewLocation.Y += MoveDir2D.Y * StepDist;

		const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));

		const float SampledZ = SampleGroundNavMeshZ(FVector(NewLocation.X, NewLocation.Y, TargetCorner.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 14.0f);
	}

	NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, CurrentGroundZ + GroundHeightOffset, DeltaTime, 16.0f);
	SetActorLocation(NewLocation);
}

void AEnemyBatteryThief::ReturnToPatrol()
{
	TargetLaserCylinder = nullptr;
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
		const FVector FinalDropLocation = IsValid(TargetDropSpotActor)
			? TargetDropSpotActor->GetActorLocation()
			: FVector(GetActorLocation().X, GetActorLocation().Y, CurrentGroundZ);

		// Place the battery cleanly at the closest BP_BatteryDropSpot actor so the player can collect it again
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

	// ==========================================
	// STATE 1: Patrolling the NavMesh
	// ==========================================
	if (CurrentState == EBatteryThiefState::Patrolling)
	{
		// Scan for any laser gun with a working battery
		if (ARedPowerCylinder* WorkingLaser = FindNearestWorkingBatteryLaser())
		{
			TargetLaserCylinder = WorkingLaser;
			CurrentState = EBatteryThiefState::Approaching;
			WaitTimer = 0.0f;
			PathRepathTimer = 0.5f;
			RefreshEnemySettings();
			BuildPathToWorldLocation(TargetLaserCylinder->GetActorLocation());

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					3101,
					3.0f,
					FColor::Orange,
					FString::Printf(TEXT("[SABOTEUR ALERT] Battery Thief detected active Laser (ID: %d) and is moving to remove its battery!"), TargetLaserCylinder->CellID)
				);
			}
			return;
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

		// Re-verify or find the closest BP_BatteryDropSpot on the NavMesh
		if (!IsValid(TargetDropSpotActor))
		{
			TargetDropSpotActor = FindClosestBatteryDropSpot();
		}

		const FVector DropTargetLocation = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		const FVector MyLoc = GetActorLocation();
		const float Dist2DToDropOff = FVector::Dist2D(MyLoc, DropTargetLocation);

		// Once we reach the closest BP_BatteryDropSpot, drop the battery there and return to patrolling
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
	// STATE 3: Approaching the nearest working battery laser
	// ==========================================
	if (CurrentState == EBatteryThiefState::Approaching)
	{
		// If the battery was removed by the player or stopped powering the laser, resume patrolling!
		if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
		{
			ReturnToPatrol();
			return;
		}

		// Also check if another working battery laser is now even closer
		if (ARedPowerCylinder* NearestNow = FindNearestWorkingBatteryLaser())
		{
			if (NearestNow != TargetLaserCylinder)
			{
				TargetLaserCylinder = NearestNow;
				BuildPathToWorldLocation(TargetLaserCylinder->GetActorLocation());
			}
		}

		const FVector MyLoc = GetActorLocation();
		const FVector LaserLoc = TargetLaserCylinder->GetActorLocation();
		const float Dist2DToLaser = FVector::Dist2D(MyLoc, LaserLoc);

		// Check if we have reached interaction range to start the 10-second removal timer
		if (Dist2DToLaser <= RemovalInteractionDistance)
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
					FString::Printf(TEXT("[BATTERY REMOVAL STARTED] Removing working battery from Laser (ID: %d)... %.0f seconds remaining!"),
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
			BuildPathToWorldLocation(LaserLoc);
		}

		FollowCurrentNavPath(DeltaTime, PatrolSpeed);
		return;
	}

	// ==========================================
	// STATE 4: Removing the working battery (10-second countdown timer)
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

		// Face the target laser cylinder while disconnecting the battery
		const FVector ToLaser2D(TargetLaserCylinder->GetActorLocation().X - GetActorLocation().X,
			TargetLaserCylinder->GetActorLocation().Y - GetActorLocation().Y, 0.0f);
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
