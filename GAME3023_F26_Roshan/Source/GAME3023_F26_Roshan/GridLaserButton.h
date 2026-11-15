// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GridLaserButton.h
// Description: Security Grid Laser Toggle Button actor.
// This is the C++ base actor for BP_GridLaserButton placed in the level.
// When the player interacts with or steps on this button, it toggles the security
// laser grid (AGridLaser) on or off!
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLaserButton.generated.h"

UCLASS()
class GAME3023_F26_ROSHAN_API AGridLaserButton : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridLaserButton();

protected:
	// Called when the game starts or when spawned into the level
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
