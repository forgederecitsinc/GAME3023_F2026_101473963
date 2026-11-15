// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: EnemyBatteryThief.cpp
// Description: Implementation of the Ground Battery Saboteur / Thief enemy.
// Handles ground NavMesh navigation, targeting powered laser cylinders,
// 10-second detachment countdown, backpack socketing, walking to drop spots,
// anti-stuck watchdog, and touch-to-follow player behavior.
// =============================================================================

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

	// Note: We do NOT create or force static mesh assets in C++ so the student can attach
	// their own custom spider / crawler mesh and materials directly in BP_EnemyBatteryThief!

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
	MovementComponent->NavAgentProps.AgentRadius = 34.0f;
	MovementComponent->NavAgentProps.AgentHeight = 44.0f;

	// Overlap trigger so physical touch by the player triggers follow mode
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

void AEnemyBatteryThief::MarkAsSpawnedUnit(const FVector& InHomeOrigin)
{
	bIsSpawnedChildUnit = true;
	bActAsSpawner = false;
	bSnapToSpawnLocationOnStart = false;

	InitialSpawnZ = SampleGroundNavMeshZ(InHomeOrigin);
	CurrentGroundZ = InitialSpawnZ;
	bHasValidGroundZ = true;
	PatrolOrigin = FVector(InHomeOrigin.X, InHomeOrigin.Y, InitialSpawnZ);
	SetActorLocation(FVector(InHomeOrigin.X, InHomeOrigin.Y, InitialSpawnZ + GroundHeightOffset));

	LastStuckCheckPosition = GetActorLocation();
	StuckWatchdogTimer = 0.0f;
	WallContactTimer = 0.0f;
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
	InitialSpawnZ = SampleGroundNavMeshZ(StartPos);
	CurrentGroundZ = InitialSpawnZ;
	bHasValidGroundZ = true;

	if (!bIsSpawnedChildUnit)
	{
		// Use the actor's placed/spawned location on the NavMesh as the patrol origin
		PatrolOrigin = FVector(StartPos.X, StartPos.Y, InitialSpawnZ);
	}

	// Snap cleanly onto the NavMesh surface
	SetActorLocation(FVector(StartPos.X, StartPos.Y, InitialSpawnZ + GroundHeightOffset));

	LastStuckCheckPosition = GetActorLocation();
	StuckWatchdogTimer = 0.0f;
	WallContactTimer = 0.0f;

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
	TargetPlayerPawn = nullptr;
	bIsFollowingPlayer = false;

	// Notify the GameMode / Spawner so a replacement spawns in 60 seconds
	OnBatteryThiefKilled.Broadcast(this);

	Destroy();
}

void AEnemyBatteryThief::KillPermanently()
{
	if (bIsDead)
	{
		return;
	}

	// Reduce max enemies in GameMode so neither initial wave nor 60s timer respawns this thief
	if (UWorld* World = GetWorld())
	{
		if (AGAME3023_F26_RoshanGameMode* GM = Cast<AGAME3023_F26_RoshanGameMode>(World->GetAuthGameMode()))
		{
			GM->MaxActiveBatteryThieves = FMath::Max(0, GM->MaxActiveBatteryThieves - 1);
		}
	}

	// Unbind all listeners so no respawn timer is scheduled
	OnBatteryThiefKilled.Clear();

	// Kill thief (drops carried battery and destroys actor)
	KillBatteryThief();
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
		NewThief->FollowPlayerSpeedBonus = FollowPlayerSpeedBonus;
		NewThief->RefreshEnemySettings();
		NewThief->MarkAsSpawnedUnit(FVector(SpawnLocation.X, SpawnLocation.Y, GroundZAtSpawn));
		NewThief->PickNextNavMeshPatrolDestination();
	}

	return NewThief;
}

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

bool AEnemyBatteryThief::IsGroundNavPathValid(const UNavigationPath* NavPath) const
{
	if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial() || NavPath->PathPoints.Num() < 2)
	{
		return false;
	}

	// Verify the path follows pedestrian walkable surfaces (stairs, ramps, walkways, other sides) and does NOT scale vertical walls
	for (int32 i = 0; i < NavPath->PathPoints.Num() - 1; ++i)
	{
		const FVector& A = NavPath->PathPoints[i];
		const FVector& B = NavPath->PathPoints[i + 1];

		const float Dist2D = FVector::Dist2D(A, B);
		const float DeltaZ = FMath::Abs(B.Z - A.Z);

		// An abrupt vertical jump > 80 cm with almost no horizontal movement is scaling a sheer vertical wall, not stairs!
		if (Dist2D < 10.0f && DeltaZ > 80.0f)
		{
			return false;
		}

		// Stair slope: stairs can be steep (e.g. up to 60-65 degrees, tan(65) ~ 2.14).
		// Only reject if it's steeper than ~68 degrees (slope > 2.5) AND significant height delta > 60 cm
		if (Dist2D > 1.0f && DeltaZ > 60.0f && (DeltaZ / Dist2D) > 2.5f)
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
		return GetActorLocation();
	}

	UWorld* World = GetWorld();
	const FVector CylinderBase = Cylinder->GetActorLocation();

	// 1. Project cylinder base directly to NavMesh with generous vertical extent (finds floor under cylinder upstairs or downstairs)
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

		// 2. Line trace down from above the cylinder base to find the exact platform/floor surface
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

	// Find the walkable platform/floor center where the cylinder stands
	const FVector TriggerGroundCenter = GetCylinderTriggerGroundCenter(Cylinder);
	const FVector ToThief2D = (PathStartPos - TriggerGroundCenter).GetSafeNormal2D();
	const float BaseAngleRad = ToThief2D.IsNearlyZero() ? 0.0f : FMath::Atan2(ToThief2D.Y, ToThief2D.X);

	// Build candidate points on the walkable surface around the cylinder
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

		// Must be on a walkable surface
		if (!IsPointOnWalkableSurface(ProjectedTarget.Location))
		{
			continue;
		}

		// Must be close enough horizontally and vertically to the cylinder on its platform
		if (FVector::Dist2D(ProjectedTarget.Location, TriggerGroundCenter) > MaxAllowedReach2D ||
			FMath::Abs(ProjectedTarget.Location.Z - TriggerGroundCenter.Z) > 150.0f)
		{
			continue;
		}

		// If the thief is already standing at this approach point
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

		// Thieves carrying a stolen battery or busy following the player are occupied
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

	// Evaluate every BP_BatteryDropSpot on the NavMesh and pick the closest reachable one (including up stairs or across the map)
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

	// 2. Roam all over the NavMesh (stairs, upstairs walkways, other sides of the arena, ground floor)
	TArray<FVector, TInlineAllocator<16>> AnchorPoints;
	AnchorPoints.Add(PathStartPos);

	// Collect cylinders (which are distributed upstairs and across different sides of the arena)
	TArray<AActor*> ArenaCylinders;
	UGameplayStatics::GetAllActorsOfClass(World, ARedPowerCylinder::StaticClass(), ArenaCylinders);
	for (AActor* Act : ArenaCylinders)
	{
		if (IsValid(Act))
		{
			AnchorPoints.Add(Act->GetActorLocation());
		}
	}

	// Collect drop spots (placed on both sides of the arena)
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

		// Alternate strategies:
		// Strategy 0: Sample near an arena anchor (cylinders / drop spots upstairs and on other sides)
		// Strategy 1: Wide roam from current position (encourages climbing stairs and exploring walkways)
		// Strategy 2: Random reachable point across the entire NavMesh
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

bool AEnemyBatteryThief::HandleStuckWatchdog(float DeltaTime, float SpeedToUse, const FVector& CurrentLocation)
{
	const float DistMoved2D = FVector::Dist2D(CurrentLocation, LastStuckCheckPosition);
	if (DistMoved2D < (SpeedToUse * DeltaTime * 0.20f))
	{
		StuckWatchdogTimer += DeltaTime;
	}
	else
	{
		StuckWatchdogTimer = FMath::Max(0.0f, StuckWatchdogTimer - DeltaTime * 2.0f);
		LastStuckCheckPosition = CurrentLocation;
	}

	if (StuckWatchdogTimer < 0.35f)
	{
		return false;
	}

	StuckWatchdogTimer = 0.0f;
	WallContactTimer = 0.0f;
	LastStuckCheckPosition = CurrentLocation;

	if (CurrentState == EBatteryThiefState::Patrolling)
	{
		CurrentPathPoints.Reset();
		CurrentPathCornerIndex = 0;
		WaitTimer = 0.0f;
		PickNextNavMeshPatrolDestination();
		return true;
	}

	if (CurrentPathPoints.IsValidIndex(CurrentPathCornerIndex + 1))
	{
		++CurrentPathCornerIndex;
		return true;
	}

	PathRepathTimer = 0.0f;
	if (CurrentState == EBatteryThiefState::Approaching && IsValid(TargetLaserCylinder))
	{
		BuildPathToWorldLocation(CurrentLaserGroundApproachPoint);
	}
	else if (CurrentState == EBatteryThiefState::Carrying)
	{
		TargetDropSpotActor = FindClosestBatteryDropSpot();
		const FVector DropLoc = IsValid(TargetDropSpotActor) ? TargetDropSpotActor->GetActorLocation() : PatrolOrigin;
		BuildPathToWorldLocation(DropLoc);
	}
	else if (CurrentState == EBatteryThiefState::FollowingPlayer && IsValid(TargetPlayerPawn))
	{
		BuildPathToWorldLocation(TargetPlayerPawn->GetActorLocation());
	}

	return true;
}

void AEnemyBatteryThief::MoveWithWallSlide(const FVector& CurrentLocation, const FVector& MoveDir2D, float StepDist, float TargetZ, bool bHasNextCorner, float DeltaTime)
{
	FVector DesiredNewXY(
		CurrentLocation.X + MoveDir2D.X * StepDist,
		CurrentLocation.Y + MoveDir2D.Y * StepDist,
		TargetZ
	);

	UWorld* World = GetWorld();
	if (!World)
	{
		WallContactTimer = 0.0f;
		SetActorLocation(DesiredNewXY);
		return;
	}

	const float SweepHeightZ = TargetZ + FMath::Max(24.0f, GroundCollisionRadius + 4.0f);
	const FVector SweepStart(CurrentLocation.X, CurrentLocation.Y, SweepHeightZ);
	const FVector SweepEnd(DesiredNewXY.X, DesiredNewXY.Y, SweepHeightZ);

	FCollisionQueryParams SweepParams(SCENE_QUERY_STAT(BatteryThiefWallSweep), false, this);
	if (IsValid(StolenBattery))
	{
		SweepParams.AddIgnoredActor(StolenBattery);
	}

	FHitResult WallHit;
	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(GroundCollisionRadius);
	const bool bHitWall = World->SweepSingleByChannel(WallHit, SweepStart, SweepEnd, FQuat::Identity, ECC_WorldStatic, SphereShape, SweepParams);

	AActor* HitActor = WallHit.GetActor();
	const bool bIgnoreHit = HitActor && (HitActor->IsA(AEnemyBatteryThief::StaticClass()) || HitActor->IsA(ABatteryCapsule::StaticClass()));
	// Only treat steep/vertical surfaces (Normal.Z < 0.6) as walls so stairs and slopes stay walkable.
	if (!bHitWall || bIgnoreHit || !WallHit.bBlockingHit || WallHit.Normal.Z >= 0.6f)
	{
		WallContactTimer = 0.0f;
		SetActorLocation(DesiredNewXY);
		return;
	}

	WallContactTimer += DeltaTime;
	if (bHasNextCorner)
	{
		++CurrentPathCornerIndex;
		WallContactTimer = 0.0f;
	}

	const float SafeFrac = FMath::Max(0.0f, WallHit.Time - 0.02f);
	const FVector SafeStopXY(
		SweepStart.X + (SweepEnd.X - SweepStart.X) * SafeFrac,
		SweepStart.Y + (SweepEnd.Y - SweepStart.Y) * SafeFrac,
		TargetZ
	);

	const FVector WallNormal2D = FVector(WallHit.Normal.X, WallHit.Normal.Y, 0.0f).GetSafeNormal();
	const FVector SlideDir2D = FVector::VectorPlaneProject(MoveDir2D, WallNormal2D).GetSafeNormal();
	const float RemainingDist = StepDist * (1.0f - WallHit.Time);

	FVector FinalPos = SafeStopXY;
	bool bSlideSucceeded = false;
	if (!SlideDir2D.IsNearlyZero() && RemainingDist > 0.5f)
	{
		const FVector SlideStart(SafeStopXY.X, SafeStopXY.Y, SweepHeightZ);
		const FVector SlideEnd = SlideStart + SlideDir2D * RemainingDist;
		FHitResult SlideHit;
		if (!World->SweepSingleByChannel(SlideHit, SlideStart, SlideEnd, FQuat::Identity, ECC_WorldStatic, SphereShape, SweepParams))
		{
			FinalPos.X = SlideEnd.X;
			FinalPos.Y = SlideEnd.Y;
			bSlideSucceeded = true;
		}
	}

	FinalPos.X += WallNormal2D.X * 5.0f;
	FinalPos.Y += WallNormal2D.Y * 5.0f;
	FinalPos.Z = TargetZ;
	SetActorLocation(FinalPos);

	if (!bSlideSucceeded || WallContactTimer > 0.2f)
	{
		WallContactTimer = 0.0f;
		if (CurrentState == EBatteryThiefState::Patrolling)
		{
			CurrentPathPoints.Reset();
			CurrentPathCornerIndex = 0;
			PickNextNavMeshPatrolDestination();
		}
		else
		{
			PathRepathTimer = 0.0f;
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
	if (HandleStuckWatchdog(DeltaTime, SpeedToUse, CurrentLocation))
	{
		return;
	}

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

	MoveWithWallSlide(CurrentLocation, MoveDir2D, StepDist, TargetZ, bHasNextCorner, DeltaTime);
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
			: GetActorLocation();
		const float DropZ = SampleGroundNavMeshZ(RawDropLocation);
		const FVector FinalDropLocation(RawDropLocation.X, RawDropLocation.Y, DropZ);

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

void AEnemyBatteryThief::PinToGroundSurface()
{
	const FVector CurrentLoc = GetActorLocation();
	const float SurfaceZ = SampleGroundNavMeshZ(CurrentLoc);
	CurrentGroundZ = SurfaceZ;
	SetActorLocation(FVector(CurrentLoc.X, CurrentLoc.Y, SurfaceZ + GroundHeightOffset));
}

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

			LiveWireUtils::ShowScreenMessage(
				3101, 3.0f, FColor::Orange,
				FString::Printf(TEXT("[SABOTEUR ALERT] Nearest Battery Thief detected active Laser (ID: %d) and is routing along the ground to steal its battery!"), TargetLaserCylinder->CellID));
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

		LiveWireUtils::ShowScreenMessage(
			3102, 2.0f, FColor::Red,
			FString::Printf(TEXT("[BATTERY REMOVAL STARTED] Removing working battery from Laser (ID: %d) from ground level... %.0f seconds remaining!"),
				TargetLaserCylinder->CellID, RemovalTimerRemaining),
			1.25f);
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

void AEnemyBatteryThief::TickRemoving(float DeltaTime)
{
	if (!DoesLaserHaveWorkingBattery(TargetLaserCylinder))
	{
		LiveWireUtils::ShowScreenMessage(
			3102, 2.5f, FColor::Green,
			TEXT("[SABOTAGE INTERRUPTED] Battery no longer on laser — Battery Thief returning to patrol."));
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

	LiveWireUtils::ShowScreenMessage(
		3102, 0.15f, FColor::Red,
		FString::Printf(TEXT("[SABOTAGING LASER %d] Removing Battery... %.1fs remaining"),
			TargetLaserCylinder->CellID, FMath::Max(0.0f, RemovalTimerRemaining)),
		1.25f);

	if (RemovalTimerRemaining <= 0.0f)
	{
		CompleteBatteryRemoval();
	}
}

void AEnemyBatteryThief::TickFollowingPlayer(float DeltaTime)
{
	const AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetPlayerPawn);
	if (!IsValid(TargetPlayerPawn) || (PlayerChar && PlayerChar->IsDead()))
	{
		StopFollowingPlayerAndResumePatrol();
		return;
	}

	const FVector PlayerLoc = TargetPlayerPawn->GetActorLocation();
	const float Dist2DToPlayer = FVector::Dist2D(GetActorLocation(), PlayerLoc);
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

	LiveWireUtils::ShowScreenMessage(
		3107, 0.15f, FColor::Red,
		FString::Printf(TEXT("[BATTERY THIEF FOLLOWING PLAYER] Touched by player! Speed: %.0f (Base: %.0f + %.0f) | Dist: %.0f"),
			FollowSpeed, PatrolSpeed, FollowPlayerSpeedBonus, Dist2DToPlayer),
		1.2f);
}

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

	// Reset any in-progress battery removal interaction
	TargetLaserCylinder = nullptr;
	RemovalTimerRemaining = 0.0f;
	CurrentLaserGroundApproachPoint = FVector::ZeroVector;

	// Clear current path and trigger immediate path computation
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

void AEnemyBatteryThief::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
