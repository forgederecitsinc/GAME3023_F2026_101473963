#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/TimerHandle.h"
#include "GAME3023_F26_RoshanGameMode.generated.h"

class AEnemyBatteryThief;

UCLASS(abstract)
class AGAME3023_F26_RoshanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGAME3023_F26_RoshanGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	bool bEnableBatteryThiefSpawning = true; //toggle battery thief spawning

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TArray<FVector> BatteryThiefSpawnLocations; //world location battery thieves will spawn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActivePerSpawnLocation = 2; //per location only 2 comes out and stays active

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "100"))
	int32 MaxTotalSpawnsPerLocation = 10; //max spawns until it stops

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1", ClampMax = "50"))
	int32 MaxActiveBatteryThieves = 6; //max in the whole arena

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "0.5"))
	float InitialWaveSpawnInterval = 5.0f; //time between spawning thieves at the same location

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "0.5"))
	float MinSpawnIntervalPerLocation = 5.0f; //minimum 5s interval between spawns at the same location

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "1.0"))
	float ReplacementRespawnDelay = 60.0f; //replacement spawns in 60s after one dies

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning", meta = (ClampMin = "10.0"))
	float BatteryThiefMoveSpeed = 300.0f; //move speed

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
		double LastSpawnTime = -100.0;
		TArray<TWeakObjectPtr<AEnemyBatteryThief>> ActiveThieves;
	};

	int32 GetSpawnSlotCount() const;
	FVector GetSpawnLocationForSlot(int32 SlotIndex) const;
	int32 GetActiveCountForSlot(int32 SlotIndex) const;
	bool IsSpawnLocationOccupied(const FVector& Location, float ClearRadius = 110.0f) const;
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
