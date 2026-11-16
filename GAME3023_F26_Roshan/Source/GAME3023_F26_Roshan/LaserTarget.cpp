#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "LiveWireUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Sets up target mesh and collision trigger
ALaserTarget::ALaserTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

// GetColorForID: updates dynamic material colors based on ID
FLinearColor ALaserTarget::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

// ApplyColors: updates dynamic material colors based on ID
void ALaserTarget::ApplyColors()
{
}

// SetLaserHit: controls laser trace and hit detection
void ALaserTarget::SetLaserHit(bool bHit, AActor* SourceActor)
{
	if (bIsHitByLaser == bHit)
	{
		return;
	}

	bIsHitByLaser = bHit;
	OnLaserHitStateChanged.Broadcast(this, bIsHitByLaser);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

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
