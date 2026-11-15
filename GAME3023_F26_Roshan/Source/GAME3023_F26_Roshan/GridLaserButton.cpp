// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GridLaserButton.cpp
// Description: Implementation of the Grid Laser Toggle Button.
// The visual mesh and interaction events are hooked up in the Blueprint child
// (BP_GridLaserButton) so designers can wire it up to any AGridLaser in the level!
// =============================================================================

#include "GridLaserButton.h"

// -----------------------------------------------------------------------------
// Constructor: sets up default properties
// -----------------------------------------------------------------------------
AGridLaserButton::AGridLaserButton()
{
	// Set this actor to call Tick() every frame if needed (can turn off for optimization)
	PrimaryActorTick.bCanEverTick = true;
}

// -----------------------------------------------------------------------------
// BeginPlay: runs once when the game begins or actor spawns
// -----------------------------------------------------------------------------
void AGridLaserButton::BeginPlay()
{
	Super::BeginPlay();
}

// -----------------------------------------------------------------------------
// Tick: runs every frame
// -----------------------------------------------------------------------------
void AGridLaserButton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
