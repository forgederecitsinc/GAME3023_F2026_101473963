// roshan - 101473963 - game3023 game engines 3
// enemy drone implementation - empty static mesh, instance-editable hover height,
// movement speed, downward laser spotlight intensity, systematic NavMesh area coverage,
// following the player exclusively when detected under the downward light cone,
// and dropping damaging laser/bomb particles at an instance-editable interval while in range

#include "EnemyDrone.h"
#include "DroneLaserParticle.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AEnemyDrone::AEnemyDrone()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Empty static mesh component so custom meshes can be attached or assigned freely
	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(SceneRoot);
	DroneMesh->SetCollisionProfileName(TEXT("NoCollision"));

	// Downward-pointing laser-like spotlight
	DownwardLaserLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("DownwardLaserLight"));
	DownwardLaserLight->SetupAttachment(DroneMesh);
	DownwardLaserLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Point straight down (-Z)
	DownwardLaserLight->Intensity = LaserLightIntensity;
	DownwardLaserLight->SetLightColor(LaserLightColor);
	DownwardLaserLight->InnerConeAngle = LaserInnerConeAngle;
	DownwardLaserLight->OuterConeAngle = LaserOuterConeAngle;
	DownwardLaserLight->AttenuationRadius = LaserAttenuationRadius;
	DownwardLaserLight->bUseInverseSquaredFalloff = false;
	DownwardLaserLight->LightFalloffExponent = 2.0f;
	DownwardLaserLight->SetCastShadows(true);

	LaserParticleClass = ADroneLaserParticle::StaticClass();
}

void AEnemyDrone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDroneSettings();
}

#if WITH_EDITOR
void AEnemyDrone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshDroneSettings();
}
#endif

void AEnemyDrone::RefreshDroneSettings()
{
	if (DownwardLaserLight)
	{
		DownwardLaserLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		DownwardLaserLight->SetIntensity(LaserLightIntensity);
		DownwardLaserLight->SetLightColor(LaserLightColor);
		DownwardLaserLight->SetInnerConeAngle(LaserInnerConeAngle);
		DownwardLaserLight->SetOuterConeAngle(FMath::Max(LaserInnerConeAngle + 0.5f, LaserOuterConeAngle));
		DownwardLaserLight->SetAttenuationRadius(FMath::Max(LaserAttenuationRadius, PatrolHeight * 2.0f));
	}
}

void AEnemyDrone::BeginPlay()
{
	Super::BeginPlay();

	RefreshDroneSettings();

	const FVector StartLocation = GetActorLocation();
	CurrentGroundZ = SampleNavMeshGroundZ(StartLocation);
	bHasValidGroundZ = true;

	PatrolOrigin = FVector(StartLocation.X, StartLocation.Y, CurrentGroundZ);

	// Snap initial Z to the configured PatrolHeight above the NavMesh
	SetActorLocation(FVector(StartLocation.X, StartLocation.Y, CurrentGroundZ + PatrolHeight));

	BuildNavMeshCoveragePoints();
	AdvanceToNextPatrolDestination();
}

bool AEnemyDrone::IsPawnUnderLight(const APawn* TargetPawn) const
{
	if (!IsValid(TargetPawn) || !DownwardLaserLight || LaserLightIntensity <= 0.0f)
	{
		return false;
	}

	const FVector LightPos = DownwardLaserLight->GetComponentLocation();
	const FVector LightDir = DownwardLaserLight->GetForwardVector().GetSafeNormal();
	const FVector PawnPos = TargetPawn->GetActorLocation();

	const FVector ToPawn = PawnPos - LightPos;
	const float DistAlongAxis = FVector::DotProduct(ToPawn, LightDir);

	// The player must be below the light source and within its attenuation range
	const float MaxRange = DownwardLaserLight->AttenuationRadius;
	if (DistAlongAxis <= 0.0f || DistAlongAxis > MaxRange)
	{
		return false;
	}

	// Calculate the spotlight cone radius at the player's vertical depth + capsule padding
	const float OuterAngleDeg = FMath::Clamp(DownwardLaserLight->OuterConeAngle, 0.5f, 85.0f);
	const float OuterAngleRad = FMath::DegreesToRadians(OuterAngleDeg);
	const float ConeRadiusAtDepth = DistAlongAxis * FMath::Tan(OuterAngleRad) + DetectionRadiusPadding;

	// Perpendicular distance from the center axis of the downward light beam to the player
	const FVector ClosestPointOnAxis = LightPos + (LightDir * DistAlongAxis);
	const float RadialDistSq = FVector::DistSquared(PawnPos, ClosestPointOnAxis);

	if (RadialDistSq > FMath::Square(ConeRadiusAtDepth))
	{
		return false;
	}

	// Verify line-of-sight so ceiling/roof obstacles block the light beam from seeing the player
	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneLightSightTrace), false, this);
		QueryParams.AddIgnoredActor(TargetPawn);

		if (World->LineTraceSingleByChannel(Hit, LightPos, PawnPos, ECC_Visibility, QueryParams))
		{
			return false;
		}
	}

	return true;
}

void AEnemyDrone::DropLaserParticle(APawn* TargetPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ADroneLaserParticle> SpawnClass = LaserParticleClass ? *LaserParticleClass : ADroneLaserParticle::StaticClass();
	const FVector SpawnLoc = DownwardLaserLight ? DownwardLaserLight->GetComponentLocation() : GetActorLocation();

	// Aim the dropped laser/bomb particle toward the detected player underneath the light cone
	FVector DropDir(0.0f, 0.0f, -1.0f);
	if (IsValid(TargetPawn))
	{
		const FVector ToPlayer = TargetPawn->GetActorLocation() - SpawnLoc;
		if (!ToPlayer.IsNearlyZero())
		{
			DropDir = ToPlayer.GetSafeNormal();
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ADroneLaserParticle* Particle = World->SpawnActor<ADroneLaserParticle>(SpawnClass, SpawnLoc, DropDir.Rotation(), SpawnParams))
	{
		Particle->InitializeParticle(LaserParticleDamage, LaserParticleSpeed, DropDir, LaserLightColor);
	}
}

float AEnemyDrone::SampleNavMeshGroundZ(const FVector& WorldPos) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return WorldPos.Z - PatrolHeight;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedLocation;
		const FVector QueryExtent(300.0f, 300.0f, 1500.0f);
		if (NavSys->ProjectPointToNavigation(WorldPos, ProjectedLocation, QueryExtent))
		{
			return ProjectedLocation.Location.Z;
		}
	}

	// Fallback downward line trace if momentarily outside NavMesh projection bounds
	FHitResult Hit;
	const FVector TraceStart = WorldPos + FVector(0.0f, 0.0f, 500.0f);
	const FVector TraceEnd = WorldPos - FVector(0.0f, 0.0f, 2500.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneGroundTrace), false, this);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.ImpactPoint.Z;
	}

	return bHasValidGroundZ ? CurrentGroundZ : (WorldPos.Z - PatrolHeight);
}

void AEnemyDrone::BuildNavMeshCoveragePoints()
{
	PatrolGroundPoints.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	// 1. If the user assigned explicit custom patrol actors in the instance, use those first
	for (AActor* WaypointActor : CustomPatrolWaypoints)
	{
		if (IsValid(WaypointActor))
		{
			FVector Loc = WaypointActor->GetActorLocation();
			if (NavSys)
			{
				FNavLocation NavLoc;
				if (NavSys->ProjectPointToNavigation(Loc, NavLoc, FVector(400.0f, 400.0f, 1500.0f)))
				{
					Loc = NavLoc.Location;
				}
			}
			PatrolGroundPoints.Add(Loc);
		}
	}

	if (PatrolGroundPoints.Num() > 0)
	{
		return;
	}

	// 2. Otherwise, systematically sample the NavMesh across a boustrophedon (lawnmower) grid
	// so the drone visits and covers every reachable area of the NavMesh
	if (NavSys)
	{
		const float Step = FMath::Max(150.0f, CoverageGridSpacing);
		const int32 HalfSteps = FMath::Clamp(FMath::CeilToInt(PatrolRadius / Step), 1, 25);
		const float MinDistSq = FMath::Square(Step * 0.45f);
		const FVector ProjectionExtent(Step * 0.55f, Step * 0.55f, 1500.0f);

		bool bReverseRow = false;
		for (int32 XIdx = -HalfSteps; XIdx <= HalfSteps; ++XIdx)
		{
			for (int32 YStep = -HalfSteps; YStep <= HalfSteps; ++YStep)
			{
				const int32 YIdx = bReverseRow ? -YStep : YStep;
				const FVector SampleOffset(XIdx * Step, YIdx * Step, 0.0f);

				if (SampleOffset.SizeSquared2D() > FMath::Square(PatrolRadius))
				{
					continue;
				}

				const FVector CandidateWorld = PatrolOrigin + SampleOffset;
				FNavLocation ProjectedNav;
				if (NavSys->ProjectPointToNavigation(CandidateWorld, ProjectedNav, ProjectionExtent))
				{
					// Avoid adding duplicate points when adjacent grid cells snap to the same NavMesh edge
					bool bTooClose = false;
					for (const FVector& Existing : PatrolGroundPoints)
					{
						if (FVector::DistSquared2D(Existing, ProjectedNav.Location) < MinDistSq &&
							FMath::Abs(Existing.Z - ProjectedNav.Location.Z) < 150.0f)
						{
							bTooClose = true;
							break;
						}
					}

					if (!bTooClose)
					{
						// Verify the point is reachable along the NavMesh from PatrolOrigin
						UNavigationPath* TestPath = NavSys->FindPathToLocationSynchronously(World, PatrolOrigin, ProjectedNav.Location, this);
						if (TestPath && TestPath->IsValid() && !TestPath->IsPartial() && TestPath->PathPoints.Num() > 0)
						{
							PatrolGroundPoints.Add(ProjectedNav.Location);
						}
					}
				}
			}
			bReverseRow = !bReverseRow;
		}
	}
}

void AEnemyDrone::AdvanceToNextPatrolDestination()
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

	// Re-populate coverage points if needed
	if (PatrolGroundPoints.Num() == 0)
	{
		BuildNavMeshCoveragePoints();
	}

	// Try to find a valid NavMesh path to the next point in our coverage cycle
	if (PatrolGroundPoints.Num() > 0)
	{
		const int32 NumPoints = PatrolGroundPoints.Num();
		for (int32 Attempt = 0; Attempt < NumPoints; ++Attempt)
		{
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % NumPoints;
			const FVector TargetGroundPos = PatrolGroundPoints[CurrentPatrolIndex];

			if (FVector::DistSquared2D(CurrentGroundPos, TargetGroundPos) < FMath::Square(60.0f))
			{
				continue;
			}

			if (NavSys)
			{
				UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, CurrentGroundPos, TargetGroundPos, this);
				if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
				{
					CurrentPathPoints = NavPath->PathPoints;
					CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
					return;
				}
			}

			// Direct fallback if NavMesh path query wasn't available
			CurrentPathPoints.Add(TargetGroundPos);
			CurrentPathCornerIndex = 0;
			return;
		}
	}

	// Fallback: pick a random reachable point on the NavMesh if grid sampling yielded nothing
	if (NavSys)
	{
		FNavLocation RandomNavPoint;
		if (NavSys->GetRandomReachablePointInRadius(CurrentGroundPos, PatrolRadius, RandomNavPoint))
		{
			UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, CurrentGroundPos, RandomNavPoint.Location, this);
			if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
			{
				CurrentPathPoints = NavPath->PathPoints;
				CurrentPathCornerIndex = (CurrentPathPoints.Num() > 1) ? 1 : 0;
			}
			else
			{
				CurrentPathPoints.Add(RandomNavPoint.Location);
				CurrentPathCornerIndex = 0;
			}
		}
	}
}

void AEnemyDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ParticleDropCooldown > 0.0f)
	{
		ParticleDropCooldown -= DeltaTime;
	}

	const FVector CurrentLocation = GetActorLocation();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	// Check whether the player is currently inside the downward light cone
	const bool bPlayerInLight = IsPawnUnderLight(PlayerPawn);

	if (bPlayerInLight && PlayerPawn)
	{
		bIsFollowingPlayer = true;
		WaitTimer = 0.0f;

		// Drop laser/bomb particles at the instance-editable interval while the player remains in the light range
		if (ParticleDropCooldown <= 0.0f)
		{
			DropLaserParticle(PlayerPawn);
			ParticleDropCooldown = FMath::Max(0.05f, LaserParticleInterval);
		}

		const FVector PlayerLoc = PlayerPawn->GetActorLocation();
		const FVector ToPlayer2D(PlayerLoc.X - CurrentLocation.X, PlayerLoc.Y - CurrentLocation.Y, 0.0f);
		const float DistToPlayer2D = ToPlayer2D.Size();
		const float StepDist = MovementSpeed * DeltaTime;

		FVector NewLocation = CurrentLocation;

		if (DistToPlayer2D > 5.0f)
		{
			const FVector MoveDir2D = ToPlayer2D / DistToPlayer2D;
			const float MoveAmount = FMath::Min(StepDist, DistToPlayer2D);
			NewLocation.X += MoveDir2D.X * MoveAmount;
			NewLocation.Y += MoveDir2D.Y * MoveAmount;

			// Smoothly rotate the drone on Yaw to face the player while keeping the light pointing straight down
			const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));
		}

		// Keep hovering at PatrolHeight above the NavMesh ground underneath
		const float SampledZ = SampleNavMeshGroundZ(FVector(NewLocation.X, NewLocation.Y, PlayerLoc.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 10.0f);
		NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, CurrentGroundZ + PatrolHeight, DeltaTime, 12.0f);

		SetActorLocation(NewLocation);
		return;
	}

	// If the player just stepped out of the light, stop dropping particles and resume the NavMesh patrol route
	if (bIsFollowingPlayer)
	{
		bIsFollowingPlayer = false;
		if (PatrolGroundPoints.IsValidIndex(CurrentPatrolIndex))
		{
			CurrentPatrolIndex = FMath::Max(-1, CurrentPatrolIndex - 1);
		}
		AdvanceToNextPatrolDestination();
	}

	if (WaitTimer > 0.0f)
	{
		WaitTimer -= DeltaTime;
		return;
	}

	if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
	{
		AdvanceToNextPatrolDestination();
		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			return;
		}
	}

	const FVector TargetCornerGround = CurrentPathPoints[CurrentPathCornerIndex];

	// Horizontal movement toward current NavMesh path corner
	const FVector ToTarget2D(TargetCornerGround.X - CurrentLocation.X, TargetCornerGround.Y - CurrentLocation.Y, 0.0f);
	const float Dist2D = ToTarget2D.Size();
	const float StepDist = MovementSpeed * DeltaTime;

	FVector NewLocation = CurrentLocation;

	if (Dist2D <= FMath::Max(StepDist, 15.0f))
	{
		NewLocation.X = TargetCornerGround.X;
		NewLocation.Y = TargetCornerGround.Y;
		CurrentGroundZ = TargetCornerGround.Z;

		++CurrentPathCornerIndex;
		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			WaitTimer = WaypointWaitTime;
			AdvanceToNextPatrolDestination();
		}
	}
	else
	{
		const FVector MoveDir2D = ToTarget2D / Dist2D;
		NewLocation.X += MoveDir2D.X * StepDist;
		NewLocation.Y += MoveDir2D.Y * StepDist;

		// Smoothly rotate drone on Yaw to face movement direction while keeping downward light vertical
		const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));

		// Sample the NavMesh ground height at our new XY and smoothly maintain PatrolHeight above it
		const float SampledZ = SampleNavMeshGroundZ(FVector(NewLocation.X, NewLocation.Y, TargetCornerGround.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 10.0f);
	}

	// Keep the drone hovering at the instance-editable PatrolHeight above the NavMesh surface
	const float TargetHoverZ = CurrentGroundZ + PatrolHeight;
	NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetHoverZ, DeltaTime, 12.0f);

	SetActorLocation(NewLocation);
}
