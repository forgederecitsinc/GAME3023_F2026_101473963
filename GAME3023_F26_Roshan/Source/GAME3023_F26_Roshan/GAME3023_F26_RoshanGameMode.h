// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GAME3023_F26_RoshanGameMode.h
// Description: LiveWire Substation Game Mode.
// Handles level rules, game loop, and enemy population:
// - Spawns Battery Thieves from the ground hatch at (X=-878, Y=-3679, Z=10)
// - Spawns 1 enemy every 10 seconds until the arena cap (5) is reached
// - When an enemy is killed, waits 60 seconds before spawning a replacement!
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "GAME3023_F26_RoshanGameMode.generated.h"

class AEnemyBatteryThief;

/**
 * AGAME3023_F26_RoshanGameMode
 * Base GameMode for our level. Controls enemy wave pacing and respawns.
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Constructor
	AGAME3023_F26_RoshanGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// =========================================================================
	// 🎮 TWEAKABLE ENEMY SPAWNING SETTINGS (Change these in BP or details!)
	// =========================================================================

	// ✏️ CHANGE THIS: Toggle automatic spawning of Battery Thieves on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	bool bEnableBatteryThiefSpawning = true;

	// ✏️ CHANGE THIS: World coordinates of the hatches where Battery Thieves crawl out from
	// Defaults to (X=-878, Y=-3679, Z=100), (X=1700, Y=2900, Z=100), and (X=-1700, Y=2900, Z=100).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TArray<FVector> BatteryThiefSpawnLocations;

	// Fallback spawn location used if BatteryThiefSpawnLocations is empty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 100.0f);

	// ✏️ CHANGE THIS: Maximum number of Battery Thieves allowed active at the same time per spawn location
	// Default is 2 (2 stay inside the level per spawn location).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActivePerSpawnLocation = 2;

	// ✏️ CHANGE THIS: Maximum total Battery Thieves each spawn location can ever spawn before stopping
	// Default is 10 per spawn location.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxTotalSpawnsPerLocation = 10;

	// ✏️ CHANGE THIS: Maximum number of Battery Thieves allowed active at the same time (legacy/global reference)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveBatteryThieves = 6;

	// ✏️ CHANGE THIS: Delay in seconds between spawning initial enemies until reaching 5
	// Default is 10.0 seconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "0.5"))
	float InitialWaveSpawnInterval = 10.0f;

	// ✏️ CHANGE THIS: Delay in seconds to spawn a replacement after an enemy is killed
	// Default is 60.0 seconds. Lower this (e.g. 15.0s) for faster respawns!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1.0"))
	float ReplacementRespawnDelay = 60.0f;

	// ✏️ CHANGE THIS: Ground movement speed enforced on all spawned Battery Thieves (cm/s)
	// Default is 300.0 cm/s.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "10.0"))
	float BatteryThiefMoveSpeed = 300.0f;

	// Class of Battery Thief to spawn (defaults to BP_EnemyBatteryThief)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TSubclassOf<AEnemyBatteryThief> BatteryThiefClass;

	// =========================================================================
	// 🧠 PUBLIC HELPERS & SPAWNING API
	// =========================================================================

	// Returns the current number of living Battery Thieves currently active in the arena
	UFUNCTION(BlueprintPure, Category = "LiveWire|Battery Thief Spawning")
	int32 GetActiveBatteryThiefCount() const;

	// Spawns a single Battery Thief at the spawn hatch if current count < MaxActiveBatteryThieves
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Battery Thief Spawning")
	AEnemyBatteryThief* TrySpawnBatteryThief();

protected:
	// Callback triggered when any Battery Thief is destroyed/killed in combat
	UFUNCTION()
	void OnSpawnedBatteryThiefKilled(AEnemyBatteryThief* KilledThief);

	// Timer callback for initial wave spawning (every 10 seconds)
	void HandleInitialWaveSpawnTick();

	// Timer callback when replacement timer expires for a specific spawn location slot
	void HandleReplacementRespawnForSlot(int32 SlotIndex);

private:
	struct FBatteryThiefSpawnSlotState
	{
		int32 TotalSpawned = 0;
		int32 PendingReplacements = 0;
		TArray<TWeakObjectPtr<AEnemyBatteryThief>> ActiveThieves;
	};

	int32 GetSpawnSlotCount() const;
	FVector GetSpawnLocationForSlot(int32 SlotIndex) const;
	int32 GetActiveCountForSlot(int32 SlotIndex) const;
	int32 FindNearestSpawnSlotIndex(const FVector& Location) const;
	AEnemyBatteryThief* TrySpawnBatteryThiefAtSlot(int32 SlotIndex);
	void RegisterThief(AEnemyBatteryThief* Thief, int32 SlotIndex, bool bAddToSlotTracking);
	bool AreAllSlotsInitialWaveComplete() const;

	// True once the initial wave has finished deploying
	bool bInitialWaveCompleted = false;

	// Per-spawn-location tracking (active count, lifetime spawned count, pending replacements)
	TArray<FBatteryThiefSpawnSlotState> SpawnSlotStates;

	// Maps each spawned/registered Battery Thief to its spawn location slot index
	TMap<TWeakObjectPtr<AEnemyBatteryThief>, int32> ThiefToSpawnSlotMap;

	// Round-robin index into BatteryThiefSpawnLocations
	int32 NextSpawnLocationIndex = 0;

	// Timer handles
	FTimerHandle InitialSpawnTimerHandle;
	TArray<FTimerHandle> ReplacementTimerHandles;
};
