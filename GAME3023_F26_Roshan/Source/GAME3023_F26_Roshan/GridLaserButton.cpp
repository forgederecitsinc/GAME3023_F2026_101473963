#include "GridLaserButton.h"

// Sets up button mesh and interaction trigger volume
AGridLaserButton::AGridLaserButton()
{
	PrimaryActorTick.bCanEverTick = true;
}

// Binds overlap events and initializes button state
void AGridLaserButton::BeginPlay()
{
	Super::BeginPlay();
}

// Handles button press animations or glowing state
void AGridLaserButton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
