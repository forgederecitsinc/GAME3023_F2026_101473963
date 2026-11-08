// roshan - 101473963 - game3023 game engines 3
// enemy drone implementation - empty static mesh, hovers at Z >= 1000,
// downward laser spotlight, systematic NavMesh area coverage,
// 5-second escape countdown when catching the player (escaped by hiding under something),
// and diving down to latch onto the player and kill them if they don't hide in 5 seconds

#include "EnemyDrone.h"
#include "DroneLaserParticle.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "BatteryCapsule.h"
#include "RedPowerCylinder.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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

float AEnemyDrone::ComputeHoverAltitudeZ() const
{
	// Always stay at Z = 1000 or higher
	const float EnforcedMinZ = FMath::Max(1000.0f, MinimumFlightZ);
	return FMath::Max(EnforcedMinZ, CurrentGroundZ + FMath::Max(1000.0f, PatrolHeight));
}

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
		DownwardLaserLight->SetAttenuationRadius(FMath::Max3(LaserAttenuationRadius, PatrolHeight * 2.0f, 2200.0f));
	}
}

void AEnemyDrone::BeginPlay()
{
	Super::BeginPlay();

	// Ensure any existing Blueprint instance that saved PatrolHeight = 350 is upgraded to Z >= 1000
	MinimumFlightZ = FMath::Max(1000.0f, MinimumFlightZ);
	if (PatrolHeight < 1000.0f)
	{
		PatrolHeight = 1000.0f;
	}

	RefreshDroneSettings();

	const FVector StartLocation = GetActorLocation();
	CurrentGroundZ = SampleNavMeshGroundZ(StartLocation);
	InitialGroundZ = CurrentGroundZ;
	bHasValidGroundZ = true;

	PatrolOrigin = FVector(StartLocation.X, StartLocation.Y, CurrentGroundZ);

	// Snap initial Z to >= 1000 above the NavMesh and record starting transform for respawn reset
	const float InitialHoverZ = FMath::Max(ComputeHoverAltitudeZ(), StartLocation.Z);
	InitialSpawnLocation = FVector(StartLocation.X, StartLocation.Y, InitialHoverZ);
	InitialSpawnRotation = GetActorRotation();
	SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	DroneState = EEnemyDroneState::Patrolling;
	BuildNavMeshCoveragePoints();
	AdvanceToNextPatrolDestination();
}

void AEnemyDrone::ResetToStartAndResumePatrol()
{
	// Detach from the player character if latched on
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// Restore initial state and timers
	DroneState = EEnemyDroneState::Patrolling;
	bIsFollowingPlayer = false;
	EscapeTimerRemaining = 0.0f;
	WaitTimer = 0.0f;
	ParticleDropCooldown = 0.0f;
	CurrentGroundZ = InitialGroundZ;
	bHasValidGroundZ = true;
	CurrentPatrolIndex = -1;

	// Teleport back to the exact starting point and rotation where the drone began
	SetActorLocationAndRotation(InitialSpawnLocation, InitialSpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// Restore normal laser light intensity/settings and resume NavMesh patrol from the start
	RefreshDroneSettings();
	AdvanceToNextPatrolDestination();
}

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

	// 1. Check straight up above the player's head for any roof, catwalk, arch, or overhang
	FHitResult OverheadHit;
	if (World->LineTraceSingleByChannel(OverheadHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = OverheadHit.GetActor();
		if (HitActor && !HitActor->IsA(ADroneLaserParticle::StaticClass()) && !HitActor->IsA(ABatteryCapsule::StaticClass()))
		{
			return true;
		}
	}

	// 2. Also check direct line of sight from the drone's spotlight to the player
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

	// If the player is hiding under something (roof, catwalk, cover), they cannot be caught by the light
	if (IsPawnHidingUnderCover(TargetPawn))
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

void AEnemyDrone::LatchAndKillPlayer(APawn* TargetPawn)
{
	if (!IsValid(TargetPawn))
	{
		return;
	}

	DroneState = EEnemyDroneState::LatchedKilled;
	bIsFollowingPlayer = false;

	// Latch directly onto the player's head/upper torso
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

		// Inflict lethal damage to kill the player immediately
		PlayerChar->ApplyDamageToPlayer(PlayerChar->MaxHealth * 10.0f, this);
	}
	else
	{
		AttachToActor(TargetPawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
		UGameplayStatics::ApplyDamage(TargetPawn, 9999.0f, GetInstigatorController(), this, UDamageType::StaticClass());
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			4201,
			5.0f,
			FColor::Red,
			TEXT("[DRONE LATCHED!] Failed to hide in 5 seconds — Enemy Drone latched onto you and killed you!"),
			true,
			FVector2D(1.4f, 1.4f)
		);
	}
}

float AEnemyDrone::SampleNavMeshGroundZ(const FVector& WorldPos) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedLocation;
		const FVector QueryExtent(300.0f, 300.0f, 2000.0f);
		if (NavSys->ProjectPointToNavigation(FVector(WorldPos.X, WorldPos.Y, CurrentGroundZ), ProjectedLocation, QueryExtent))
		{
			return ProjectedLocation.Location.Z;
		}
	}

	// Fallback downward line trace to find floor Z below the drone
	FHitResult Hit;
	const FVector TraceStart(WorldPos.X, WorldPos.Y, FMath::Max(WorldPos.Z, 1200.0f));
	const FVector TraceEnd(WorldPos.X, WorldPos.Y, -2000.0f);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DroneGroundTrace), false, this);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Hit.ImpactPoint.Z;
	}

	return bHasValidGroundZ ? CurrentGroundZ : 0.0f;
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

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(PlayerPawn);

	// While latched onto the killed player, wait until the player respawns; once alive again, detach and reset to start
	if (DroneState == EEnemyDroneState::LatchedKilled)
	{
		if (!IsValid(PlayerPawn) || (PlayerChar && !PlayerChar->IsDead()))
		{
			ResetToStartAndResumePatrol();
		}
		return;
	}

	if (ParticleDropCooldown > 0.0f)
	{
		ParticleDropCooldown -= DeltaTime;
	}

	const FVector CurrentLocation = GetActorLocation();

	// ==========================================
	// STATE 3: Diving Down To Latch Onto Player & Kill Them (5s timer expired)
	// ==========================================
	if (DroneState == EEnemyDroneState::DivingToLatch)
	{
		if (!IsValid(PlayerPawn) || (PlayerChar && PlayerChar->IsDead()))
		{
			ResetToStartAndResumePatrol();
			return;
		}

		// Target the player's upper body / head to latch on
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

		const FRotator DiveRot(0.0f, DiveDir.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), DiveRot, DeltaTime, TurnSpeed * 2.0f));
		return;
	}

	// ==========================================
	// STATE 2: Player Caught — 5-Second Escape Countdown (Hide Under Something!)
	// ==========================================
	if (DroneState == EEnemyDroneState::TrackingPlayer)
	{
		const bool bPlayerDead = PlayerChar && PlayerChar->IsDead();
		if (bPlayerDead)
		{
			ResetToStartAndResumePatrol();
			return;
		}

		// Check if the player escaped by hiding under a roof, catwalk, or overhead structure
		if (!IsValid(PlayerPawn) || IsPawnHidingUnderCover(PlayerPawn))
		{
			DroneState = EEnemyDroneState::Patrolling;
			bIsFollowingPlayer = false;
			EscapeTimerRemaining = 0.0f;
			RefreshDroneSettings();

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					4201,
					3.0f,
					FColor::Green,
					TEXT("[ESCAPED DRONE!] You hid under cover and escaped the Enemy Drone!"),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}

			if (PatrolGroundPoints.IsValidIndex(CurrentPatrolIndex))
			{
				CurrentPatrolIndex = FMath::Max(-1, CurrentPatrolIndex - 1);
			}
			AdvanceToNextPatrolDestination();
			return;
		}

		// Count down the 5-second escape window
		EscapeTimerRemaining -= DeltaTime;

		// Pulse the downward laser light rapidly to warn the player during the 5-second countdown
		if (DownwardLaserLight && GetWorld())
		{
			const float Pulse = 0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 16.0f);
			DownwardLaserLight->SetIntensity(LaserLightIntensity * (0.5f + 1.2f * Pulse));
		}

		// Drop laser particles while tracking
		if (ParticleDropCooldown <= 0.0f)
		{
			DropLaserParticle(PlayerPawn);
			ParticleDropCooldown = FMath::Max(0.05f, LaserParticleInterval);
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				4201,
				0.15f,
				FColor::Red,
				FString::Printf(TEXT("[DRONE LOCKED ON!] Hide under cover in %.1fs or the Drone will dive and latch onto you!"),
					FMath::Max(0.0f, EscapeTimerRemaining)),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		// If the 5 seconds run out and the player hasn't hidden under cover, dive down to latch and kill!
		if (EscapeTimerRemaining <= 0.0f)
		{
			DroneState = EEnemyDroneState::DivingToLatch;
			if (DownwardLaserLight)
			{
				DownwardLaserLight->SetIntensity(LaserLightIntensity * 2.0f);
			}
			return;
		}

		// Keep following directly above the player at Z >= 1000 during the 5-second escape window
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

			const FRotator DesiredRotation(0.0f, MoveDir2D.Rotation().Yaw, 0.0f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, TurnSpeed));
		}

		const float SampledZ = SampleNavMeshGroundZ(FVector(NewLocation.X, NewLocation.Y, PlayerLoc.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 10.0f);

		// Stay at Z >= 1000 while tracking
		const float TargetHoverZ = ComputeHoverAltitudeZ();
		NewLocation.Z = FMath::Max(1000.0f, FMath::FInterpTo(CurrentLocation.Z, TargetHoverZ, DeltaTime, 12.0f));

		SetActorLocation(NewLocation);
		return;
	}

	// ==========================================
	// STATE 1: Patrolling at Z >= 1000
	// ==========================================
	if (IsPawnUnderLight(PlayerPawn))
	{
		DroneState = EEnemyDroneState::TrackingPlayer;
		bIsFollowingPlayer = true;
		EscapeTimerRemaining = EscapeTimeLimit;
		WaitTimer = 0.0f;

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				4201,
				2.5f,
				FColor::Red,
				FString::Printf(TEXT("[CAUGHT BY DRONE!] Hide under something within %.0f seconds!"), EscapeTimeLimit),
				true,
				FVector2D(1.35f, 1.35f)
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

		// Sample the NavMesh ground height at our new XY
		const float SampledZ = SampleNavMeshGroundZ(FVector(NewLocation.X, NewLocation.Y, TargetCornerGround.Z));
		CurrentGroundZ = FMath::FInterpTo(CurrentGroundZ, SampledZ, DeltaTime, 10.0f);
	}

	// Always stay at Z = 1000 or higher while patrolling
	const float TargetHoverZ = ComputeHoverAltitudeZ();
	NewLocation.Z = FMath::Max(1000.0f, FMath::FInterpTo(CurrentLocation.Z, TargetHoverZ, DeltaTime, 12.0f));

	SetActorLocation(NewLocation);
}
