// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GAME3023_F26_RoshanGameMode.generated.h"

class AEnemyBatteryThief;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AGAME3023_F26_RoshanGameMode();

	virtual void BeginPlay() override;

	// If true, automatically spawns a Battery Thief at DefaultBatteryThiefSpawnLocation when the level starts if none is placed in the level yet
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	bool bAutoSpawnBatteryThiefIfNoneInLevel = true;

	// Default spawn location for the Battery Thief (hatch at X=-878, Y=-3679, Z=10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	FVector DefaultBatteryThiefSpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	// Class of Battery Thief to spawn (defaults to BP_EnemyBatteryThief if available, otherwise AEnemyBatteryThief)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Battery Thief Spawning")
	TSubclassOf<AEnemyBatteryThief> BatteryThiefClass;
};
