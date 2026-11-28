#include "LiveWireUtils.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

namespace LiveWireUtils
{
	// ProjectToNavMesh: moves a world position onto the nearest valid navigation area.
	bool ProjectToNavMesh(const UWorld* World, const FVector& WorldPos, const FVector& Extent, FVector& OutLocation)
	{
		if (!World)
		{
			return false;
		}

		const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!NavSys)
		{
			return false;
		}

		FNavLocation Projected;
		if (NavSys->ProjectPointToNavigation(WorldPos, Projected, Extent))
		{
			OutLocation = Projected.Location;
			return true;
		}
		return false;
	}

	// SampleWalkableGroundZ: finds the ground height for AI or actor placement.
	float SampleWalkableGroundZ(const UWorld* World, const FVector& WorldPos, const AActor* IgnoreActor, float MinNormalZ)
	{
		FVector Projected;
		if (ProjectToNavMesh(World, WorldPos, FVector(100.0f, 100.0f, 350.0f), Projected))
		{
			return Projected.Z;
		}

		if (!World)
		{
			return WorldPos.Z;
		}

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LiveWireGroundSample), false, IgnoreActor);
		const FVector TraceStart = WorldPos + FVector(0.0f, 0.0f, 80.0f);
		const FVector TraceEnd = WorldPos - FVector(0.0f, 0.0f, 350.0f);

		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params) && Hit.Normal.Z >= MinNormalZ)
		{
			return Hit.ImpactPoint.Z;
		}

		return WorldPos.Z;
	}
}
