#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "LiveWireUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Constructor: creates the target and sets up its root component.
ALaserTarget::ALaserTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	// Use a scene component as the target's main transform.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

// GetColorForID: asks the utility class for the color matching this target ID.
FLinearColor ALaserTarget::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

// ApplyColors: placeholder for adding visual color changes to the target.
void ALaserTarget::ApplyColors()
{
}

// SetLaserHit: changes the hit state and updates matching transformer bushings.
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

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find all transformer bushings and update the one with the same target ID.
	TArray<AActor*> FoundBushings;
	UGameplayStatics::GetAllActorsOfClass(World, ATransformerBushing::StaticClass(), FoundBushings);
	for (AActor* Actor : FoundBushings)
	{
		if (ATransformerBushing* Bushing = Cast<ATransformerBushing>(Actor))
		{
			if (Bushing->BushingID == TargetID)
			{
				Bushing->SetLitUp(bIsHitByLaser);
			}
		}
	}
}
