// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAME3023_F26_RoshanGameMode.h"
#include "EnemyBatteryThief.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

AGAME3023_F26_RoshanGameMode::AGAME3023_F26_RoshanGameMode()
{
	// Automatically find BP_EnemyBatteryThief so the user's Blueprint version is spawned by default
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

void AGAME3023_F26_RoshanGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!bEnableBatteryThiefSpawning)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Subscribe to any Battery Thieves already placed in the level and ensure their speed is 300
	TArray<AActor*> ExistingThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), ExistingThieves);
	for (AActor* Act : ExistingThieves)
	{
		if (AEnemyBatteryThief* Existing = Cast<AEnemyBatteryThief>(Act))
		{
			Existing->PatrolSpeed = BatteryThiefMoveSpeed;
			Existing->CarrySpeed = BatteryThiefMoveSpeed;
			Existing->RefreshEnemySettings();
			Existing->OnBatteryThiefKilled.RemoveDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
			Existing->OnBatteryThiefKilled.AddDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
		}
	}

	// Spawn the first Battery Thief immediately if none exist yet
	if (ExistingThieves.Num() < MaxActiveBatteryThieves)
	{
		TrySpawnBatteryThief();
	}

	if (GetActiveBatteryThiefCount() >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
	}
	else
	{
		// Spawn one Battery Thief every 10 seconds until 5 are active in the NavMesh
		World->GetTimerManager().SetTimer(
			InitialSpawnTimerHandle,
			this,
			&AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick,
			InitialWaveSpawnInterval,
			true
		);
	}
}

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

AEnemyBatteryThief* AGAME3023_F26_RoshanGameMode::TrySpawnBatteryThief()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const int32 CurrentAlive = GetActiveBatteryThiefCount();
	if (CurrentAlive >= MaxActiveBatteryThieves)
	{
		return nullptr;
	}

	TSubclassOf<AEnemyBatteryThief> SpawnClass = BatteryThiefClass ? *BatteryThiefClass : AEnemyBatteryThief::StaticClass();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEnemyBatteryThief* SpawnedThief = World->SpawnActor<AEnemyBatteryThief>(SpawnClass, DefaultBatteryThiefSpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (SpawnedThief)
	{
		SpawnedThief->PatrolSpeed = BatteryThiefMoveSpeed;
		SpawnedThief->CarrySpeed = BatteryThiefMoveSpeed;
		SpawnedThief->RefreshEnemySettings();
		SpawnedThief->MarkAsSpawnedUnit(DefaultBatteryThiefSpawnLocation);
		SpawnedThief->PickNextNavMeshPatrolDestination();

		SpawnedThief->OnBatteryThiefKilled.RemoveDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
		SpawnedThief->OnBatteryThiefKilled.AddDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
	}

	return SpawnedThief;
}

void AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Account for both currently alive thieves and any killed ones that already have a 60s replacement queued
	const int32 TotalSlotsUsed = GetActiveBatteryThiefCount() + PendingReplacementCount;
	if (TotalSlotsUsed >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
		World->GetTimerManager().ClearTimer(InitialSpawnTimerHandle);
		return;
	}

	TrySpawnBatteryThief();

	if (GetActiveBatteryThiefCount() + PendingReplacementCount >= MaxActiveBatteryThieves)
	{
		bInitialWaveCompleted = true;
		World->GetTimerManager().ClearTimer(InitialSpawnTimerHandle);
	}
}

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

	// Schedule a replacement Battery Thief to spawn in 60 seconds
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

void AGAME3023_F26_RoshanGameMode::HandleReplacementRespawn()
{
	PendingReplacementCount = FMath::Max(0, PendingReplacementCount - 1);
	TrySpawnBatteryThief();
}
