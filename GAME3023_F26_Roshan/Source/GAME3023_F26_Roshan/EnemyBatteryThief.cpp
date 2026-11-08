// roshan - 101473963 - game3023 game engines 3
// ground battery saboteur / battery thief enemy implementation
// stage 1: spawns at (X=-878.0, Y=-3679.0, Z=10.0) and patrols the ground NavMesh

#include "EnemyBatteryThief.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
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
		ScannerLight->SetLightColor(ScannerLightColor);
		ScannerLight->SetInnerConeAngle(ScannerOuterConeAngle * 0.6f);
		ScannerLight->SetOuterConeAngle(ScannerOuterConeAngle);
		ScannerLight->SetAttenuationRadius(ScannerLightRange);
	}

	if (MovementComponent)
	{
		MovementComponent->MaxSpeed = PatrolSpeed;
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

	// Project our current position onto the NavMesh so pathfinding always starts from a valid NavMesh polygon
	// (especially when emerging from a recessed wall hatch like X=-878, Y=-3679, Z=10)
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

			FVector TargetLoc = WaypointActor->GetActorLocation();
			if (NavSys)
			{
				FNavLocation ProjectedTarget;
				if (NavSys->ProjectPointToNavigation(TargetLoc, ProjectedTarget, FVector(400.0f, 400.0f, 500.0f)))
				{
					TargetLoc = ProjectedTarget.Location;
				}

				UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, PathStartPos, TargetLoc, this);
				if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
				{
					CurrentPathPoints = NavPath->PathPoints;
					CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
					return;
				}
			}

			CurrentPathPoints.Add(TargetLoc);
			CurrentPathCornerIndex = 0;
			return;
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
				// Ensure the chosen patrol point is a meaningful distance away
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

		// Secondary fallback: any navigable point in radius
		FNavLocation FallbackNavPoint;
		if (NavSys->GetRandomPointInNavigableRadius(PathStartPos, PatrolRadius, FallbackNavPoint))
		{
			CurrentPathPoints.Add(FallbackNavPoint.Location);
			CurrentPathCornerIndex = 0;
		}
	}
}

void AEnemyBatteryThief::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetCorner = CurrentPathPoints[CurrentPathCornerIndex];

	const FVector ToTarget2D(TargetCorner.X - CurrentLocation.X, TargetCorner.Y - CurrentLocation.Y, 0.0f);
	const float Dist2D = ToTarget2D.Size();
	const float StepDist = PatrolSpeed * DeltaTime;

	FVector NewLocation = CurrentLocation;

	if (Dist2D <= FMath::Max(StepDist, AcceptanceRadius))
	{
		// Reached current NavMesh path corner; advance to next corner along the ground corridor
		++CurrentPathCornerIndex;
		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			WaitTimer = PatrolPointWaitTime;
			PickNextNavMeshPatrolDestination();
			return;
		}
	}
	else
	{
		const FVector MoveDir2D = ToTarget2D / Dist2D;
		NewLocation.X += MoveDir2D.X * StepDist;
		NewLocation.Y += MoveDir2D.Y * StepDist;

		// Smoothly rotate the ground crawler on Yaw to face its movement direction
		const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));

		// Sample the NavMesh ground height at the new XY so the enemy stays grounded on ramps/floors
		const float SampledZ = SampleGroundNavMeshZ(FVector(NewLocation.X, NewLocation.Y, TargetCorner.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 14.0f);
	}

	NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, CurrentGroundZ + GroundHeightOffset, DeltaTime, 16.0f);
	SetActorLocation(NewLocation);
}

void AEnemyBatteryThief::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
