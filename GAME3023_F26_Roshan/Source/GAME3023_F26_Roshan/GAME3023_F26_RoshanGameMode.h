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

	// ✏️ CHANGE THIS: World coordinates of the hatch where Battery Thieves crawl out from
	// Default location is (X=-878, Y=-3679, Z=10) on the ground floor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	// ✏️ CHANGE THIS: Maximum number of Battery Thieves allowed active at the same time
	// Default is 5. Increase if you want a more crowded, intense arena!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveBatteryThieves = 5;

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

	// Timer callback when 60-second replacement timer expires
	void HandleReplacementRespawn();

private:
	// True once the initial 5 enemies have finished spawning
	bool bInitialWaveCompleted = false;

	// Number of killed enemies currently waiting on their 60-second replacement timer
	int32 PendingReplacementCount = 0;

	// Timer handles
	FTimerHandle InitialSpawnTimerHandle;
	TArray<FTimerHandle> ReplacementTimerHandles;
};
