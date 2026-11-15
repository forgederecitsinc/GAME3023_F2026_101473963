// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3

#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "LiveWireUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ALaserTarget::ALaserTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

FLinearColor ALaserTarget::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

void ALaserTarget::ApplyColors()
{
	// Materials are authored directly in BP_LaserTarget and preserved in C++.
}

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
