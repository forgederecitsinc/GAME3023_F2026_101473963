#include "GAME3023_F26_RoshanGameMode.h"
#include "EnemyBatteryThief.h"
#include "LiveWireUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

AGAME3023_F26_RoshanGameMode::AGAME3023_F26_RoshanGameMode()
{
	BatteryThiefSpawnLocations = {
		FVector(-878.0f, -3679.0f, 10.0f),
		FVector(1700.0f, 2900.0f, 10.0f),
		FVector(-1700.0f, 2900.0f, 10.0f)
	};

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

	//start spawning thieves
	UWorld* World = GetWorld();
	if (!bEnableBatteryThiefSpawning || !World)
	{
		return;
	}

	const int32 SlotCount = GetSpawnSlotCount(); //counts how much is spawned
	SpawnSlotStates.SetNum(SlotCount);
	ThiefToSpawnSlotMap.Empty();

	TArray<AActor*> ExistingThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), ExistingThieves);
	for (AActor* Act : ExistingThieves)
	{
		if (AEnemyBatteryThief* Existing = Cast<AEnemyBatteryThief>(Act))
		{
			const bool bTrackInSlot = (SlotCount > 0 && !Existing->bIsDead);
			const int32 NearestSlot = bTrackInSlot ? FindNearestSpawnSlotIndex(Existing->GetActorLocation()) : INDEX_NONE;
			RegisterThief(Existing, NearestSlot, bTrackInSlot);
		}
	}

	for (int32 SlotIdx = 0; SlotIdx < SlotCount; ++SlotIdx)
	{
		if (GetActiveCountForSlot(SlotIdx) < MaxActivePerSpawnLocation &&
			SpawnSlotStates[SlotIdx].TotalSpawned < MaxTotalSpawnsPerLocation)
		{
			TrySpawnBatteryThiefAtSlot(SlotIdx);
		}
	}

	if (AreAllSlotsInitialWaveComplete())
	{
		bInitialWaveCompleted = true;
		return;
	}

	World->GetTimerManager().SetTimer(
		InitialSpawnTimerHandle,
		this,
		&AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick,
		InitialWaveSpawnInterval,
		true
	);
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

int32 AGAME3023_F26_RoshanGameMode::GetSpawnSlotCount() const
{
	return BatteryThiefSpawnLocations.Num() > 0 ? BatteryThiefSpawnLocations.Num() : 1;
}

FVector AGAME3023_F26_RoshanGameMode::GetSpawnLocationForSlot(int32 SlotIndex) const
{
	if (BatteryThiefSpawnLocations.IsValidIndex(SlotIndex))
	{
		return BatteryThiefSpawnLocations[SlotIndex];
	}
	return DefaultBatteryThiefSpawnLocation;
}

int32 AGAME3023_F26_RoshanGameMode::GetActiveCountForSlot(int32 SlotIndex) const
{
	if (!SpawnSlotStates.IsValidIndex(SlotIndex))
	{
		return 0;
	}

	int32 AliveCount = 0;
	for (const TWeakObjectPtr<AEnemyBatteryThief>& WeakThief : SpawnSlotStates[SlotIndex].ActiveThieves)
	{
		const AEnemyBatteryThief* Thief = WeakThief.Get();
		if (IsValid(Thief) && !Thief->bIsDead)
		{
			++AliveCount;
		}
	}
	return AliveCount;
}

int32 AGAME3023_F26_RoshanGameMode::FindNearestSpawnSlotIndex(const FVector& Location) const
{
	const int32 SlotCount = GetSpawnSlotCount();
	int32 BestIndex = 0;
	float BestDistSq = TNumericLimits<float>::Max();

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const float DistSq = FVector::DistSquared(Location, GetSpawnLocationForSlot(i));
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = i;
		}
	}
	return BestIndex;
}

bool AGAME3023_F26_RoshanGameMode::AreAllSlotsInitialWaveComplete() const
{
	for (int32 SlotIdx = 0; SlotIdx < SpawnSlotStates.Num(); ++SlotIdx)
	{
		const FBatteryThiefSpawnSlotState& State = SpawnSlotStates[SlotIdx];
		const int32 ActiveOrPending = GetActiveCountForSlot(SlotIdx) + State.PendingReplacements;
		if (ActiveOrPending < MaxActivePerSpawnLocation && State.TotalSpawned < MaxTotalSpawnsPerLocation)
		{
			return false;
		}
	}
	return true;
}

void AGAME3023_F26_RoshanGameMode::RegisterThief(AEnemyBatteryThief* Thief, int32 SlotIndex, bool bAddToSlotTracking)
{
	if (!Thief)
	{
		return;
	}

	Thief->PatrolSpeed = BatteryThiefMoveSpeed;
	Thief->CarrySpeed = BatteryThiefMoveSpeed;
	Thief->RefreshEnemySettings();

	Thief->OnBatteryThiefKilled.RemoveDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);
	Thief->OnBatteryThiefKilled.AddDynamic(this, &AGAME3023_F26_RoshanGameMode::OnSpawnedBatteryThiefKilled);

	if (bAddToSlotTracking && SpawnSlotStates.IsValidIndex(SlotIndex))
	{
		SpawnSlotStates[SlotIndex].ActiveThieves.Add(Thief);
		++SpawnSlotStates[SlotIndex].TotalSpawned;
		ThiefToSpawnSlotMap.Add(Thief, SlotIndex);
	}
}

bool AGAME3023_F26_RoshanGameMode::IsSpawnLocationOccupied(const FVector& Location, float ClearRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float ClearRadiusSq = FMath::Square(ClearRadius);
	TArray<AActor*> FoundThieves;
	UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), FoundThieves);
	for (AActor* Act : FoundThieves)
	{
		const AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(Act);
		if (IsValid(Thief) && !Thief->bIsDead)
		{
			if (FVector::DistSquared2D(Thief->GetActorLocation(), Location) < ClearRadiusSq)
			{
				return true;
			}
		}
	}
	return false;
}

AEnemyBatteryThief* AGAME3023_F26_RoshanGameMode::TrySpawnBatteryThiefAtSlot(int32 SlotIndex)
{
	UWorld* World = GetWorld();
	if (!World || !SpawnSlotStates.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	FBatteryThiefSpawnSlotState& SlotState = SpawnSlotStates[SlotIndex];
	if (SlotState.TotalSpawned >= MaxTotalSpawnsPerLocation ||
		GetActiveCountForSlot(SlotIndex) >= MaxActivePerSpawnLocation)
	{
		return nullptr;
	}

	// Enforce at least MinSpawnIntervalPerLocation (5s) between spawns at the same location
	const double Now = World->GetTimeSeconds();
	if (SlotState.LastSpawnTime >= 0.0 && (Now - SlotState.LastSpawnTime) < static_cast<double>(MinSpawnIntervalPerLocation))
	{
		return nullptr;
	}

	FVector ChosenSpawnLocation = GetSpawnLocationForSlot(SlotIndex);
	FVector ProjectedSpawn;
	if (LiveWireUtils::ProjectToNavMesh(World, ChosenSpawnLocation, FVector(150.0f, 150.0f, 250.0f), ProjectedSpawn))
	{
		ChosenSpawnLocation = ProjectedSpawn;
	}

	// Prevent spawning on top of another thief that is still standing on the spawn pad
	if (IsSpawnLocationOccupied(ChosenSpawnLocation, 110.0f))
	{
		return nullptr;
	}

	TSubclassOf<AEnemyBatteryThief> SpawnClass = BatteryThiefClass ? *BatteryThiefClass : AEnemyBatteryThief::StaticClass();
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyBatteryThief* SpawnedThief = World->SpawnActor<AEnemyBatteryThief>(SpawnClass, ChosenSpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!SpawnedThief)
	{
		return nullptr;
	}

	SlotState.LastSpawnTime = Now;
	RegisterThief(SpawnedThief, SlotIndex, true);
	SpawnedThief->MarkAsSpawnedUnit(ChosenSpawnLocation);
	SpawnedThief->PickNextNavMeshPatrolDestination();

	return SpawnedThief;
}

AEnemyBatteryThief* AGAME3023_F26_RoshanGameMode::TrySpawnBatteryThief()
{
	const int32 SlotCount = SpawnSlotStates.Num();
	if (SlotCount <= 0)
	{
		return nullptr;
	}

	for (int32 Attempt = 0; Attempt < SlotCount; ++Attempt)
	{
		const int32 SlotIndex = (NextSpawnLocationIndex + Attempt) % SlotCount;
		if (AEnemyBatteryThief* Spawned = TrySpawnBatteryThiefAtSlot(SlotIndex))
		{
			NextSpawnLocationIndex = (SlotIndex + 1) % SlotCount;
			return Spawned;
		}
	}

	return nullptr;
}

void AGAME3023_F26_RoshanGameMode::HandleInitialWaveSpawnTick()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 SlotIdx = 0; SlotIdx < SpawnSlotStates.Num(); ++SlotIdx)
	{
		const FBatteryThiefSpawnSlotState& State = SpawnSlotStates[SlotIdx];
		const int32 ActiveOrPending = GetActiveCountForSlot(SlotIdx) + State.PendingReplacements;
		if (ActiveOrPending < MaxActivePerSpawnLocation && State.TotalSpawned < MaxTotalSpawnsPerLocation)
		{
			TrySpawnBatteryThiefAtSlot(SlotIdx);
		}
	}

	if (AreAllSlotsInitialWaveComplete())
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

	int32 SlotIndex = 0;
	if (KilledThief)
	{
		if (const int32* FoundSlot = ThiefToSpawnSlotMap.Find(KilledThief))
		{
			SlotIndex = *FoundSlot;
			ThiefToSpawnSlotMap.Remove(KilledThief);
		}
		else
		{
			SlotIndex = FindNearestSpawnSlotIndex(KilledThief->GetActorLocation());
		}

		if (SpawnSlotStates.IsValidIndex(SlotIndex))
		{
			SpawnSlotStates[SlotIndex].ActiveThieves.RemoveAll([KilledThief](const TWeakObjectPtr<AEnemyBatteryThief>& Entry)
			{
				return !Entry.IsValid() || Entry.Get() == KilledThief;
			});
		}
	}

	if (!SpawnSlotStates.IsValidIndex(SlotIndex))
	{
		return;
	}

	FBatteryThiefSpawnSlotState& SlotState = SpawnSlotStates[SlotIndex];

	if (SlotState.TotalSpawned + SlotState.PendingReplacements >= MaxTotalSpawnsPerLocation)
	{
		return;
	}

	++SlotState.PendingReplacements;

	LiveWireUtils::ShowScreenMessage(
		-1,
		4.0f,
		FColor::Cyan,
		FString::Printf(
			TEXT("[BATTERY THIEF DESTROYED] Hatch %d replacement (%d/%d) will spawn in %.0f seconds."),
			SlotIndex + 1,
			SlotState.TotalSpawned + SlotState.PendingReplacements,
			MaxTotalSpawnsPerLocation,
			ReplacementRespawnDelay
		)
	);

	FTimerHandle NewReplacementTimer;
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
		this,
		&AGAME3023_F26_RoshanGameMode::HandleReplacementRespawnForSlot,
		SlotIndex
	);
	World->GetTimerManager().SetTimer(
		NewReplacementTimer,
		RespawnDelegate,
		ReplacementRespawnDelay,
		false
	);
	ReplacementTimerHandles.Add(NewReplacementTimer);
}

void AGAME3023_F26_RoshanGameMode::HandleReplacementRespawnForSlot(int32 SlotIndex)
{
	if (!SpawnSlotStates.IsValidIndex(SlotIndex))
	{
		return;
	}

	FBatteryThiefSpawnSlotState& SlotState = SpawnSlotStates[SlotIndex];
	if (SlotState.TotalSpawned >= MaxTotalSpawnsPerLocation ||
		GetActiveCountForSlot(SlotIndex) >= MaxActivePerSpawnLocation)
	{
		SlotState.PendingReplacements = FMath::Max(0, SlotState.PendingReplacements - 1);
		return;
	}

	if (TrySpawnBatteryThiefAtSlot(SlotIndex))
	{
		SlotState.PendingReplacements = FMath::Max(0, SlotState.PendingReplacements - 1);
		return;
	}

	// Slot was occupied or within the 5s cooldown; retry after MinSpawnIntervalPerLocation
	if (UWorld* World = GetWorld())
	{
		FTimerHandle RetryTimer;
		FTimerDelegate RetryDelegate = FTimerDelegate::CreateUObject(
			this,
			&AGAME3023_F26_RoshanGameMode::HandleReplacementRespawnForSlot,
			SlotIndex
		);
		World->GetTimerManager().SetTimer(
			RetryTimer,
			RetryDelegate,
			FMath::Max(0.5f, MinSpawnIntervalPerLocation),
			false
		);
		ReplacementTimerHandles.Add(RetryTimer);
	}
}
