// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "GAME3023_F26_RoshanGameMode.generated.h"

class AEnemyBatteryThief;

/**
 *  GameMode for LiveWire Substation
 *  Manages spawning Battery Thieves out of (X=-878, Y=-3679, Z=10) every 10 seconds up to 5 active in the NavMesh,
 *  and respawning a replacement 60 seconds after any Battery Thief is killed.
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AGAME3023_F26_RoshanGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Enables automatic Battery Thief spawning from DefaultBatteryThiefSpawnLocation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	bool bEnableBatteryThiefSpawning = true;

	// Spawn location for Battery Thieves (hatch at X=-878, Y=-3679, Z=10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	// Maximum number of Battery Thieves allowed inside the NavMesh at all times (5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveBatteryThieves = 5;

	// Interval in seconds between initial spawns until 5 are active in the NavMesh (10.0 seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "0.5"))
	float InitialWaveSpawnInterval = 10.0f;

	// Delay in seconds to spawn a replacement Battery Thief after one is killed (60.0 seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1.0"))
	float ReplacementRespawnDelay = 60.0f;

	// Movement speed enforced on all spawned Battery Thieves (300.0 cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "10.0"))
	float BatteryThiefMoveSpeed = 300.0f;

	// Class of Battery Thief to spawn (defaults to BP_EnemyBatteryThief if available, otherwise AEnemyBatteryThief)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TSubclassOf<AEnemyBatteryThief> BatteryThiefClass;

	// Returns the current number of living Battery Thieves in the level
	UFUNCTION(BlueprintPure, Category = "LiveWire|Battery Thief Spawning")
	int32 GetActiveBatteryThiefCount() const;

	// Spawns a single Battery Thief at DefaultBatteryThiefSpawnLocation if below MaxActiveBatteryThieves (5)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Battery Thief Spawning")
	AEnemyBatteryThief* TrySpawnBatteryThief();

protected:
	UFUNCTION()
	void OnSpawnedBatteryThiefKilled(AEnemyBatteryThief* KilledThief);

	void HandleInitialWaveSpawnTick();
	void HandleReplacementRespawn();

private:
	// Tracks whether the initial 5 enemies have finished their 10-second wave spawning
	bool bInitialWaveCompleted = false;

	// Number of killed enemies currently waiting on a 60-second replacement timer
	int32 PendingReplacementCount = 0;

	FTimerHandle InitialSpawnTimerHandle;
	TArray<FTimerHandle> ReplacementTimerHandles;
};
