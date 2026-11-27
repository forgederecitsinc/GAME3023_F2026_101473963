#include "EnemyDrone.h"
#include "DroneLaserParticle.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "BatteryCapsule.h"
#include "RedPowerCylinder.h"
#include "LiveWireUtils.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

// Constructor: creates the drone root, mesh, scanner light, and movement components.
AEnemyDrone::AEnemyDrone()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(SceneRoot);
	DroneMesh->SetCollisionProfileName(TEXT("NoCollision"));

	DownwardLaserLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("DownwardLaserLight"));
	DownwardLaserLight->SetupAttachment(DroneMesh);
	DownwardLaserLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
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

// OnConstruction: prevents the drone from flying below its configured minimum height.
void AEnemyDrone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshDroneSettings();
}

#if WITH_EDITOR
// Validates patrol height and settings when modified in editor
void AEnemyDrone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshDroneSettings();
}
#endif

// ComputeHoverAltitudeZ: calculates a safe patrol height above the walkable ground.
float AEnemyDrone::ComputeHoverAltitudeZ() const
{
	const float EnforcedMinZ = FMath::Max(1000.0f, MinimumFlightZ);
	return FMath::Max(EnforcedMinZ, CurrentGroundZ + FMath::Max(1000.0f, PatrolHeight));
}

// RefreshDroneSettings: applies spotlight and movement values from the drone properties.
void AEnemyDrone::RefreshDroneSettings()
{
	MinimumFlightZ = FMath::Max(1000.0f, MinimumFlightZ);
	if (PatrolHeight < 1000.0f)
	{
		PatrolHeight = 1000.0f;
	}

	if (DownwardLaserLight)
	{
		DownwardLaserLight->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		DownwardLaserLight->SetIntensity(LaserLightIntensity);
		DownwardLaserLight->SetLightColor(LaserLightColor);
		DownwardLaserLight->SetInnerConeAngle(LaserInnerConeAngle);
		DownwardLaserLight->SetOuterConeAngle(FMath::Max(LaserInnerConeAngle + 0.5f, LaserOuterConeAngle));
		DownwardLaserLight->SetAttenuationRadius(FMath::Max3(LaserAttenuationRadius, PatrolHeight * 2.0f, MinimumFlightZ + 1000.0f));
	}
}

// BeginPlay: initializes patrol points and starts the drone in its patrolling state.
void AEnemyDrone::BeginPlay()
{
	Super::BeginPlay();

	MinimumFlightZ = FMath::Max(1000.0f, MinimumFlightZ);
	if (PatrolHeight < 1000.0f)
	{
		PatrolHeight = 1000.0f;
	}

	RefreshDroneSettings();

	const FVector StartLocation = GetActorLocation();
	FVector ClampedStartGround = StartLocation;
	if (ClampLocationToNavMesh(StartLocation, ClampedStartGround, 2500.0f))
	{
		CurrentGroundZ = ClampedStartGround.Z;
		PatrolOrigin = ClampedStartGround;
	}
	else
	{
		CurrentGroundZ = SampleNavMeshGroundZ(StartLocation);
		PatrolOrigin = FVector(StartLocation.X, StartLocation.Y, CurrentGroundZ);
	}
	InitialGroundZ = CurrentGroundZ;
	bHasValidGroundZ = true;

	const float InitialHoverZ = FMath::Max(ComputeHoverAltitudeZ(), StartLocation.Z);
	InitialSpawnLocation = FVector(PatrolOrigin.X, PatrolOrigin.Y, InitialHoverZ);
	InitialSpawnRotation = GetActorRotation();
	SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
	UpdateLightRangeForAltitude(InitialSpawnLocation.Z, CurrentGroundZ);

	DroneState = EEnemyDroneState::Patrolling;
	BuildNavMeshCoveragePoints();
	AdvanceToNextPatrolDestination();
}

// Teleports drone back to start point and restarts patrol path
void AEnemyDrone::ResetToStartAndResumePatrol()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	DroneState = EEnemyDroneState::Patrolling;
	bIsFollowingPlayer = false;
	EscapeTimerRemaining = 0.0f;
	WaitTimer = 0.0f;
	ParticleDropCooldown = 0.0f;
	CurrentGroundZ = InitialGroundZ;
	bHasValidGroundZ = true;
	CurrentPatrolIndex = -1;

	SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	RefreshDroneSettings();
	AdvanceToNextPatrolDestination();
}

// Checks if pawn is covered by a roof or umbrella to avoid detection
bool AEnemyDrone::IsPawnHidingUnderCover(const APawn* TargetPawn) const
{
	if (!IsValid(TargetPawn))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector PawnPos = TargetPawn->GetActorLocation();
	const FVector TraceStart = PawnPos + FVector(0.0f, 0.0f, 40.0f);
	const FVector TraceEnd = FVector(PawnPos.X, PawnPos.Y, FMath::Max(GetActorLocation().Z + 200.0f, 1500.0f));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneCoverCheck), false, this);
	QueryParams.AddIgnoredActor(TargetPawn);

	if (const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetPawn))
	{
		if (PlayerChar->GetCarriedBattery())
		{
			QueryParams.AddIgnoredActor(PlayerChar->GetCarriedBattery());
		}
	}

	FHitResult OverheadHit;
	if (World->LineTraceSingleByChannel(OverheadHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = OverheadHit.GetActor();
		if (HitActor && !HitActor->IsA(ADroneLaserParticle::StaticClass()) && !HitActor->IsA(ABatteryCapsule::StaticClass()))
		{
			return true;
		}
	}

	if (DownwardLaserLight)
	{
		FHitResult SightHit;
		const FVector LightPos = DownwardLaserLight->GetComponentLocation();
		if (World->LineTraceSingleByChannel(SightHit, LightPos, TraceStart, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = SightHit.GetActor();
			if (HitActor && !HitActor->IsA(ADroneLaserParticle::StaticClass()) && !HitActor->IsA(ABatteryCapsule::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

// Checks if target pawn is inside the drone spotlight cone
bool AEnemyDrone::IsPawnUnderLight(const APawn* TargetPawn) const
{
	if (!IsValid(TargetPawn) || !DownwardLaserLight || LaserLightIntensity <= 0.0f)
	{
		return false;
	}

	if (const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetPawn))
	{
		if (PlayerChar->IsDead())
		{
			return false;
		}
	}

	if (IsPawnHidingUnderCover(TargetPawn))
	{
		return false;
	}

	const FVector LightPos = DownwardLaserLight->GetComponentLocation();
	const FVector LightDir = DownwardLaserLight->GetForwardVector().GetSafeNormal();
	const FVector PawnPos = TargetPawn->GetActorLocation();

	const FVector ToPawn = PawnPos - LightPos;
	const float DistAlongAxis = FVector::DotProduct(ToPawn, LightDir);

	const float MaxRange = DownwardLaserLight->AttenuationRadius;
	if (DistAlongAxis <= 0.0f || DistAlongAxis > MaxRange)
	{
		return false;
	}

	const float OuterAngleDeg = FMath::Clamp(DownwardLaserLight->OuterConeAngle, 0.5f, 85.0f);
	const float OuterAngleRad = FMath::DegreesToRadians(OuterAngleDeg);
	const float ConeRadiusAtDepth = DistAlongAxis * FMath::Tan(OuterAngleRad) + DetectionRadiusPadding;

	const FVector ClosestPointOnAxis = LightPos + (LightDir * DistAlongAxis);
	const float RadialDistSq = FVector::DistSquared(PawnPos, ClosestPointOnAxis);

	if (RadialDistSq > FMath::Square(ConeRadiusAtDepth))
	{
		return false;
	}

	return true;
}

// DropLaserParticle: spawns a falling laser particle below the target pawn.
void AEnemyDrone::DropLaserParticle(APawn* TargetPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ADroneLaserParticle> SpawnClass = LaserParticleClass ? *LaserParticleClass : ADroneLaserParticle::StaticClass();
	const FVector SpawnLoc = DownwardLaserLight ? DownwardLaserLight->GetComponentLocation() : GetActorLocation();

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

// LatchAndKillPlayer: starts the diving latch sequence and kills the target player.
void AEnemyDrone::LatchAndKillPlayer(APawn* TargetPawn)
{
	if (!IsValid(TargetPawn))
	{
		return;
	}

	DroneState = EEnemyDroneState::LatchedKilled;
	bIsFollowingPlayer = false;

	if (AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetPawn))
	{
		USkeletalMeshComponent* CharMesh = PlayerChar->GetMesh();
		const FName HeadSocket(TEXT("head"));
		if (CharMesh && CharMesh->DoesSocketExist(HeadSocket))
		{
			AttachToComponent(CharMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HeadSocket);
			SetActorRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
		}
		else
		{
			AttachToComponent(PlayerChar->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			SetActorRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		}

		PlayerChar->ApplyDamageToPlayer(PlayerChar->MaxHealth * 10.0f, this);
	}
	else
	{
		AttachToActor(TargetPawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		UGameplayStatics::ApplyDamage(TargetPawn, 9999.0f, GetInstigatorController(), this, UDamageType::StaticClass());
	}
}

// ClampLocationToNavMesh: updates movement pathfinding and waypoint navigation
bool AEnemyDrone::ClampLocationToNavMesh(const FVector& WorldPos, FVector& OutNavGroundPos, float HorizontalSearchRadius) const
{
	const float QueryZ = bHasValidGroundZ ? CurrentGroundZ : WorldPos.Z;
	return LiveWireUtils::ProjectToNavMesh(
		GetWorld(),
		FVector(WorldPos.X, WorldPos.Y, QueryZ),
		FVector(HorizontalSearchRadius, HorizontalSearchRadius, 4000.0f),
		OutNavGroundPos);
}

// UpdateLightRangeForAltitude: handles update light range for altitude
void AEnemyDrone::UpdateLightRangeForAltitude(float CurrentZ, float GroundZ)
{
	if (!DownwardLaserLight)
	{
		return;
	}

	const float HeightAboveGround = FMath::Max(0.0f, CurrentZ - GroundZ);
	const float RequiredRange = FMath::Max3(
		LaserAttenuationRadius,
		FMath::Max(PatrolHeight, MinimumFlightZ) + 1000.0f,
		HeightAboveGround + 800.0f
	);

	if (DownwardLaserLight->AttenuationRadius < RequiredRange)
	{
		DownwardLaserLight->SetAttenuationRadius(RequiredRange);
	}
}

// SampleNavMeshGroundZ: updates movement pathfinding and waypoint navigation
float AEnemyDrone::SampleNavMeshGroundZ(const FVector& WorldPos) const
{
	FVector ProjectedGroundPos;
	if (ClampLocationToNavMesh(WorldPos, ProjectedGroundPos, 300.0f))
	{
		return ProjectedGroundPos.Z;
	}

	const float TracedZ = LiveWireUtils::SampleWalkableGroundZ(
		GetWorld(),
		FVector(WorldPos.X, WorldPos.Y, FMath::Max(WorldPos.Z, 1200.0f)),
		this,
		0.0f);

	return (TracedZ != 0.0f || !bHasValidGroundZ) ? TracedZ : CurrentGroundZ;
}

// BuildNavMeshCoveragePoints: updates movement pathfinding and waypoint navigation
void AEnemyDrone::BuildNavMeshCoveragePoints()
{
	PatrolGroundPoints.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	for (AActor* WaypointActor : CustomPatrolWaypoints)
	{
		if (IsValid(WaypointActor))
		{
			FVector WaypointLoc = WaypointActor->GetActorLocation();
			if (NavSys)
			{
				FNavLocation ProjectedNav;
				if (NavSys->ProjectPointToNavigation(WaypointLoc, ProjectedNav, FVector(500.0f, 500.0f, 4000.0f)))
				{
					PatrolGroundPoints.Add(ProjectedNav.Location);
				}
			}
			else
			{
				PatrolGroundPoints.Add(WaypointLoc);
			}
		}
	}

	if (PatrolGroundPoints.Num() > 0)
	{
		return;
	}

	if (!NavSys)
	{
		return;
	}

	const float Step = FMath::Max(150.0f, CoverageGridSpacing);
	const int32 HalfSteps = FMath::Clamp(FMath::CeilToInt(PatrolRadius / Step), 1, 25);
	const float MinDistSq = FMath::Square(Step * 0.45f);
	const FVector ProjectionExtent(Step * 0.45f, Step * 0.45f, 3000.0f);

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
			if (!NavSys->ProjectPointToNavigation(CandidateWorld, ProjectedNav, ProjectionExtent))
			{
				continue;
			}

			const FVector PointToAdd = ProjectedNav.Location;

			bool bTooClose = false;
			for (const FVector& Existing : PatrolGroundPoints)
			{
				if (FVector::DistSquared2D(Existing, PointToAdd) < MinDistSq)
				{
					bTooClose = true;
					break;
				}
			}

			if (!bTooClose)
			{
				PatrolGroundPoints.Add(PointToAdd);
			}
		}
		bReverseRow = !bReverseRow;
	}

	if (PatrolGroundPoints.Num() == 0)
	{
		for (int32 SampleIdx = 0; SampleIdx < 16; ++SampleIdx)
		{
			FNavLocation RandomNav;
			if (NavSys->GetRandomPointInNavigableRadius(PatrolOrigin, PatrolRadius, RandomNav))
			{
				PatrolGroundPoints.Add(RandomNav.Location);
			}
		}
	}
}

// AdvanceToNextPatrolDestination: updates movement pathfinding and waypoint navigation
void AEnemyDrone::AdvanceToNextPatrolDestination()
{
	CurrentPathPoints.Reset();
	CurrentPathCornerIndex = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	FVector GroundStart(CurrentLocation.X, CurrentLocation.Y, CurrentGroundZ);
	ClampLocationToNavMesh(CurrentLocation, GroundStart, 1000.0f);

	if (PatrolGroundPoints.Num() == 0)
	{
		BuildNavMeshCoveragePoints();
	}

	if (PatrolGroundPoints.Num() > 0)
	{
		const int32 NumPoints = PatrolGroundPoints.Num();
		for (int32 Attempt = 0; Attempt < NumPoints; ++Attempt)
		{
			CurrentPatrolIndex = (CurrentPatrolIndex + 1) % NumPoints;
			const FVector TargetGroundPos = PatrolGroundPoints[CurrentPatrolIndex];

			if (FVector::DistSquared2D(CurrentLocation, TargetGroundPos) < FMath::Square(80.0f))
			{
				continue;
			}

			if (NavSys)
			{
				if (UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, GroundStart, TargetGroundPos, this))
				{
					if (NavPath->IsValid() && NavPath->PathPoints.Num() > 0)
					{
						for (const FVector& PathPt : NavPath->PathPoints)
						{
							if (CurrentPathPoints.Num() == 0 && FVector::DistSquared2D(CurrentLocation, PathPt) < FMath::Square(25.0f))
							{
								continue;
							}
							CurrentPathPoints.Add(PathPt);
						}

						if (CurrentPathPoints.Num() > 0)
						{
							CurrentPathCornerIndex = 0;
							return;
						}
					}
				}
			}

			CurrentPathPoints.Add(TargetGroundPos);
			CurrentPathCornerIndex = 0;
			return;
		}
	}

	if (NavSys)
	{
		FNavLocation RandomNavLoc;
		if (NavSys->GetRandomPointInNavigableRadius(PatrolOrigin, PatrolRadius, RandomNavLoc))
		{
			CurrentPathPoints.Add(RandomNavLoc.Location);
			CurrentPathCornerIndex = 0;
			return;
		}
	}

	CurrentPathPoints.Add(GroundStart);
	CurrentPathCornerIndex = 0;
}

// Tick: updates the drone behavior based on its current patrol, tracking, diving, or latched state.
void AEnemyDrone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(PlayerPawn);
	const bool bIsPlayerDead = (PlayerChar && PlayerChar->IsDead());

	if (DroneState == EEnemyDroneState::LatchedKilled)
	{
		TickLatched(PlayerPawn, bIsPlayerDead);
		return;
	}

	if (ParticleDropCooldown > 0.0f)
	{
		ParticleDropCooldown -= DeltaTime;
	}

	switch (DroneState)
	{
	case EEnemyDroneState::DivingToLatch:
		TickDiving(DeltaTime, PlayerPawn, bIsPlayerDead);
		break;
	case EEnemyDroneState::TrackingPlayer:
		TickTracking(DeltaTime, PlayerPawn, bIsPlayerDead);
		break;
	case EEnemyDroneState::Patrolling:
	default:
		TickPatrolling(DeltaTime, PlayerPawn);
		break;
	}
}

// TickLatched: handles tick latched
void AEnemyDrone::TickLatched(APawn* PlayerPawn, bool bIsPlayerDead)
{
	if (!IsValid(PlayerPawn) || !bIsPlayerDead)
	{
		ResetToStartAndResumePatrol();
	}
}

// TickDiving: handles tick diving
void AEnemyDrone::TickDiving(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead)
{
	if (!IsValid(PlayerPawn) || bIsPlayerDead)
	{
		ResetToStartAndResumePatrol();
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector LatchTargetPos = PlayerPawn->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
	const FVector ToLatch = LatchTargetPos - CurrentLocation;
	const float DistToLatch = ToLatch.Size();
	const float DiveStep = DiveLatchSpeed * DeltaTime;

	if (DistToLatch <= FMath::Max(DiveStep, 65.0f))
	{
		LatchAndKillPlayer(PlayerPawn);
		return;
	}

	const FVector DiveDir = ToLatch / DistToLatch;
	SetActorLocation(CurrentLocation + DiveDir * DiveStep);
	FaceDirection2D(DiveDir, DeltaTime, 2.0f);
}

// TickTracking: handles tick tracking
void AEnemyDrone::TickTracking(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead)
{
	if (bIsPlayerDead)
	{
		ResetToStartAndResumePatrol();
		return;
	}

	if (!IsValid(PlayerPawn) || IsPawnHidingUnderCover(PlayerPawn))
	{
		DroneState = EEnemyDroneState::Patrolling;
		bIsFollowingPlayer = false;
		EscapeTimerRemaining = 0.0f;
		RefreshDroneSettings();

		if (PatrolGroundPoints.IsValidIndex(CurrentPatrolIndex))
		{
			CurrentPatrolIndex = FMath::Max(-1, CurrentPatrolIndex - 1);
		}
		AdvanceToNextPatrolDestination();
		return;
	}

	EscapeTimerRemaining -= DeltaTime;

	if (DownwardLaserLight && GetWorld())
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 16.0f);
		DownwardLaserLight->SetIntensity(LaserLightIntensity * (0.5f + 1.2f * Pulse));
	}

	if (ParticleDropCooldown <= 0.0f)
	{
		DropLaserParticle(PlayerPawn);
		ParticleDropCooldown = FMath::Max(0.05f, LaserParticleInterval);
	}

	if (EscapeTimerRemaining <= 0.0f)
	{
		DroneState = EEnemyDroneState::DivingToLatch;
		if (DownwardLaserLight)
		{
			DownwardLaserLight->SetIntensity(LaserLightIntensity * 2.0f);
		}
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const FVector ToPlayer2D(PlayerLoc.X - CurrentLocation.X, PlayerLoc.Y - CurrentLocation.Y, 0.0f);
	const float DistToPlayer2D = ToPlayer2D.Size();

	FVector TargetXY = CurrentLocation;
	if (DistToPlayer2D > 5.0f)
	{
		const FVector MoveDir2D = ToPlayer2D / DistToPlayer2D;
		const float MoveAmount = FMath::Min(MovementSpeed * DeltaTime, DistToPlayer2D);
		TargetXY.X += MoveDir2D.X * MoveAmount;
		TargetXY.Y += MoveDir2D.Y * MoveAmount;
		FaceDirection2D(MoveDir2D, DeltaTime);
	}

	UpdateHoverPosition(TargetXY, PlayerLoc.Z, DeltaTime);
}

// TickPatrolling: updates movement pathfinding and waypoint navigation
void AEnemyDrone::TickPatrolling(float DeltaTime, APawn* PlayerPawn)
{
	if (IsPawnUnderLight(PlayerPawn))
	{
		DroneState = EEnemyDroneState::TrackingPlayer;
		bIsFollowingPlayer = true;
		EscapeTimerRemaining = EscapeTimeLimit;
		WaitTimer = 0.0f;
		return;
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

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetCornerGround = CurrentPathPoints[CurrentPathCornerIndex];
	const FVector ToTarget2D(TargetCornerGround.X - CurrentLocation.X, TargetCornerGround.Y - CurrentLocation.Y, 0.0f);
	const float Dist2D = ToTarget2D.Size();
	const float StepDist = MovementSpeed * DeltaTime;

	FVector TargetXY = CurrentLocation;
	if (Dist2D <= FMath::Max(StepDist, 15.0f))
	{
		TargetXY.X = TargetCornerGround.X;
		TargetXY.Y = TargetCornerGround.Y;
		CurrentGroundZ = TargetCornerGround.Z;

		++CurrentPathCornerIndex;
		if (!CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex))
		{
			WaitTimer = WaypointWaitTime;
			AdvanceToNextPatrolDestination();
		}

		const float TargetHoverZ = ComputeHoverAltitudeZ();
		TargetXY.Z = FMath::Max(FMath::Max(1000.0f, MinimumFlightZ), FMath::FInterpTo(CurrentLocation.Z, TargetHoverZ, DeltaTime, 12.0f));
		UpdateLightRangeForAltitude(TargetXY.Z, CurrentGroundZ);
		SetActorLocation(TargetXY);
		return;
	}

	const FVector MoveDir2D = ToTarget2D / Dist2D;
	TargetXY.X += MoveDir2D.X * StepDist;
	TargetXY.Y += MoveDir2D.Y * StepDist;

	FaceDirection2D(MoveDir2D, DeltaTime);
	UpdateHoverPosition(TargetXY, TargetCornerGround.Z, DeltaTime);
}

// UpdateHoverPosition: handles update hover position
void AEnemyDrone::UpdateHoverPosition(const FVector& TargetXY, float FallbackZ, float DeltaTime)
{
	FVector NewLocation = TargetXY;
	const float SampledZ = SampleNavMeshGroundZ(FVector(NewLocation.X, NewLocation.Y, FallbackZ));

	CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 10.0f);

	const float TargetHoverZ = ComputeHoverAltitudeZ();
	NewLocation.Z = FMath::Max(FMath::Max(1000.0f, MinimumFlightZ), FMath::FInterpTo(GetActorLocation().Z, TargetHoverZ, DeltaTime, 12.0f));
	UpdateLightRangeForAltitude(NewLocation.Z, CurrentGroundZ);
	SetActorLocation(NewLocation);
}

// FaceDirection2D: handles face direction 2 d
void AEnemyDrone::FaceDirection2D(const FVector& Dir2D, float DeltaTime, float SpeedMultiplier)
{
	const FRotator DesiredRotation(0.0f, Dir2D.Rotation().Yaw, 0.0f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed * SpeedMultiplier));
}
