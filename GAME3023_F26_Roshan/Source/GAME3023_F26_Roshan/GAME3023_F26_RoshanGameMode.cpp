// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GAME3023_F26_RoshanGameMode.cpp
// Description: Implementation of the GameMode for the LiveWire Substation.
// Controls the enemy pacing loop:
// 1. Spawns one Battery Thief every 10 seconds up to 5 total in the level.
// 2. Automatically listens for death events when a thief is eliminated.
// 3. Waits 60 seconds after death to spawn a new replacement thief.
// =============================================================================

#include "GAME3023_F26_RoshanGameMode.h"
#include "EnemyBatteryThief.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

// -----------------------------------------------------------------------------
// CONSTRUCTOR: Finds the BP_EnemyBatteryThief Blueprint class
// -----------------------------------------------------------------------------
AGAME3023_F26_RoshanGameMode::AGAME3023_F26_RoshanGameMode()
{
	// Find the student's custom BP_EnemyBatteryThief blueprint so it spawns with all mesh/material setup intact!
	static ConstructorHelpers::FClassFinder<AEnemyBatteryThief> BPThiefFinder(TEXT("/Game/_Blueprints/Enemy/BP_EnemyBatteryThief"));
	if (BPThiefFinder.Succeeded())
	{
		BatteryThiefClass = BPThiefFinder.Class;
	}
	else
	{
		BatteryThiefClass = AEnemyBatteryThief::StaticClass();
	}
}

// -----------------------------------------------------------------------------
// BEGIN PLAY: Initializes existing thieves and starts the 10-second wave timer
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanGameMode::BeginPlay()
{
	Super::BeginPlay();

	// If spawner is disabled in settings, do nothing
	if (!bEnableBatteryThiefSpawning)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Check for any Battery Thieves that were already placed into the map by hand
	TArray<AActor*> ExistingThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), ExistingThieves);
	for (AActor* Act : ExistingThieves)
	{
		if (AEnemyBatteryThief* Existing = Cast<AEnemyBatteryThief>(Act))
		{
			// Enforce our global game mode movement speed
			Existing->PatrolSpeed = BatteryThiefMoveSpeed;
			Existing->CarrySpeed = BatteryThiefMoveSpeed;
			Existing->RefreshEnemySettings();

			// Hook up death notifications so we know when to queue a 60s replacement
			Existing->OnBatteryThiefKilled.RemoveDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
			Existing->OnBatteryThiefKilled.AddDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
		}
	}

	// 2. Spawn the very first Battery Thief immediately on match start if none exist
	if (ExistingThieves.Num() < MaxActiveBatteryThieves)
	{
		TrySpawnBatteryThief();
	}

	// 3. If we already have 5 active, we're done with the initial wave; otherwise set recurring 10s timer
	if (GetActiveBatteryThiefCount() >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
	}
	else
	{
		// Spawn one new Battery Thief every 10.0 seconds until 5 are active in the level
		World->GetTimerManager().SetTimer(
			InitialSpawnTimerHandle,
			this,
			&AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick,
			InitialWaveSpawnInterval,
			true
		);
	}
}

// -----------------------------------------------------------------------------
// END PLAY: Cleans up all active spawn timers when changing levels or stopping
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialSpawnTimerHandle);
		for (FTimerHandle& Handle : ReplacementTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Handle);
		}
		ReplacementTimerHandles.Empty();
	}

	Super::EndPlay(EndPlayReason);
}

// -----------------------------------------------------------------------------
// GET ACTIVE BATTERY THIEF COUNT: Counts how many thieves are alive right now
// -----------------------------------------------------------------------------
int32 AGAME3023_F26_RoshanGameMode::GetActiveBatteryThiefCount() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<AActor*> FoundThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), FoundThieves);

	int32 AliveCount = 0;
	for (AActor* Act : FoundThieves)
	{
		const AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(Act);
		if (IsValid(Thief) && !Thief->bIsDead)
		{
			++AliveCount;
		}
	}

	return AliveCount;
}

// -----------------------------------------------------------------------------
// TRY SPAWN BATTERY THIEF: Spawns a thief at the hatch if below the max cap (5)
// -----------------------------------------------------------------------------
AEnemyBatteryThief* AGAME3023_F26_RoshanGameMode::TrySpawnBatteryThief()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Do not exceed the maximum allowed enemies in the arena
	const int32 CurrentAlive = GetActiveBatteryThiefCount();
	if (CurrentAlive >= MaxActiveBatteryThieves)
	{
		return nullptr;
	}

	TSubclassOf<AEnemyBatteryThief> SpawnClass = BatteryThiefClass ? *BatteryThiefClass : AEnemyBatteryThief::StaticClass();
	FActorSpawnParameters SpawnParams;
	// Always spawn even if colliding with another actor at the hatch
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the new enemy at the designated hatch location (X=-878, Y=-3679, Z=10)
	AEnemyBatteryThief* SpawnedThief = World->SpawnActor<AEnemyBatteryThief>(SpawnClass, DefaultBatteryThiefSpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedThief)
	{
		// Configure speed, home origin, and start patrolling the NavMesh
		SpawnedThief->PatrolSpeed = BatteryThiefMoveSpeed;
		SpawnedThief->CarrySpeed = BatteryThiefMoveSpeed;
		SpawnedThief->RefreshEnemySettings();
		SpawnedThief->MarkAsSpawnedUnit(DefaultBatteryThiefSpawnLocation);
		SpawnedThief->PickNextNavMeshPatrolDestination();

		// Bind death event callback so we know when to schedule a replacement
		SpawnedThief->OnBatteryThiefKilled.RemoveDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
		SpawnedThief->OnBatteryThiefKilled.AddDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
	}

	return SpawnedThief;
}

// -----------------------------------------------------------------------------
// INITIAL WAVE TICK: Fires every 10 seconds to spawn initial enemies
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check total slots used (alive enemies + any dead ones waiting on their 60s respawn timer)
	const int32 TotalSlotsUsed = GetActiveBatteryThiefCount() + PendingReplacementCount;
	if (TotalSlotsUsed >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
		World->GetTimerManager().ClearTimer(InitialSpawnTimerHandle);
		return;
	}

	TrySpawnBatteryThief();

	// If we just hit the max cap of 5, shut off the initial wave timer
	if (GetActiveBatteryThiefCount() + PendingReplacementCount >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
		World->GetTimerManager().ClearTimer(InitialSpawnTimerHandle);
	}
}

// -----------------------------------------------------------------------------
// DEATH NOTIFICATION: Called when a Battery Thief is eliminated
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled(AEnemyBatteryThief* KilledThief)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	++PendingReplacementCount;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			4.0f,
			FColor::Cyan,
			FString::Printf(TEXT("[BATTERY THIEF DESTROYED] Replacement will spawn in %.0f seconds."), ReplacementRespawnDelay)
		);
	}

	// ⏱️ Schedule a new replacement Battery Thief to spawn after ReplacementRespawnDelay (60.0s)
	FTimerHandle NewReplacementTimer;
	World->GetTimerManager().SetTimer(
		NewReplacementTimer,
		this,
		&AGAME3023_F26_RoshanGameMode::HandleReplacementRespawn,
		ReplacementRespawnDelay,
		false
	);
	ReplacementTimerHandles.Add(NewReplacementTimer);
}

// -----------------------------------------------------------------------------
// REPLACEMENT RESPAWN: Fires when the 60-second replacement timer finishes
// -----------------------------------------------------------------------------
void AGAME3023_F26_RoshanGameMode::HandleReplacementRespawn()
{
	PendingReplacementCount = FMath::Max(0, PendingReplacementCount - 1);
	TrySpawnBatteryThief();
}
