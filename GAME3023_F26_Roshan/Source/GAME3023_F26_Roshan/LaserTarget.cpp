#include "LaserTarget.h"
#include "TransformerBushing.h"

// Constructor: creates the target and sets up its root component.
ALaserTarget::ALaserTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	// Use a scene component as the target's main transform.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

// SetLaserHit: changes the hit state and updates any linked transformer bushing.
void ALaserTarget::SetLaserHit(bool bHit, AActor* SourceActor)
{
	// Skip the update if the target's hit state is already correct.
	if (bIsHitByLaser == bHit)
	{
		return;
	}

	bIsHitByLaser = bHit;

	// Tell Blueprints and other listeners that the target state changed.
	OnLaserHitStateChanged.Broadcast(this, bIsHitByLaser);

	if (LinkedBushing)
	{
		LinkedBushing->SetLitUp(bIsHitByLaser);
	}
}
