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
	AGAME3023_F26_RoshanGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	bool bEnableBatteryThiefSpawning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TArray<FVector> BatteryThiefSpawnLocations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActivePerSpawnLocation = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxTotalSpawnsPerLocation = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveBatteryThieves = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "0.5"))
	float InitialWaveSpawnInterval = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1.0"))
	float ReplacementRespawnDelay = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "10.0"))
	float BatteryThiefMoveSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TSubclassOf<AEnemyBatteryThief> BatteryThiefClass;

	UFUNCTION(BlueprintPure, Category = "LiveWire|Battery Thief Spawning")
	int32 GetActiveBatteryThiefCount() const;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Battery Thief Spawning")
	AEnemyBatteryThief* TrySpawnBatteryThief();

protected:
	UFUNCTION()
	void OnSpawnedBatteryThiefKilled(AEnemyBatteryThief* KilledThief);

	void HandleInitialWaveSpawnTick();

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

	bool bInitialWaveCompleted = false;

	TArray<FBatteryThiefSpawnSlotState> SpawnSlotStates;

	TMap<TWeakObjectPtr<AEnemyBatteryThief>, int32> ThiefToSpawnSlotMap;

	int32 NextSpawnLocationIndex = 0;

	FTimerHandle InitialSpawnTimerHandle;
	TArray<FTimerHandle> ReplacementTimerHandles;
};
