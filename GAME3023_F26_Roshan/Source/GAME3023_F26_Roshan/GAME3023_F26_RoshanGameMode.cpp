// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAME3023_F26_RoshanGameMode.h"
#include "EnemyBatteryThief.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

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

	if (bAutoSpawnBatteryThiefIfNoneInLevel)
	{
		if (UWorld* World = GetWorld())
		{
			TArray<AActor*> ExistingThieves;
			UGameplayStatics::GetAllActorsOfClass(World, AEnemyBatteryThief::StaticClass(), ExistingThieves);

			// If no Battery Thief actor is placed in the level yet, spawn one at (X=-878, Y=-3679, Z=10)
			if (ExistingThieves.Num() == 0)
			{
				TSubclassOf<AEnemyBatteryThief> SpawnClass = BatteryThiefClass ? *BatteryThiefClass : AEnemyBatteryThief::StaticClass();
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

				World->SpawnActor<AEnemyBatteryThief>(SpawnClass, DefaultBatteryThiefSpawnLocation, FRotator::ZeroRotator, SpawnParams);
			}
		}
	}
}
