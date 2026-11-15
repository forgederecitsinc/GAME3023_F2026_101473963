#include "GridLaserButton.h"

AGridLaserButton::AGridLaserButton()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGridLaserButton::BeginPlay()
{
	Super::BeginPlay();
}

void AGridLaserButton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
