// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3

#include "LiveWireUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

namespace LiveWireUtils
{
	FLinearColor GetColorForID(int32 ID)
	{
		switch (ID)
		{
			case 1001: return FLinearColor(1.00f, 0.10f, 0.12f); // Red
			case 1002: return FLinearColor(0.08f, 0.95f, 0.18f); // Emerald Green
			case 1003: return FLinearColor(0.08f, 0.45f, 1.00f); // Cobalt Blue
			case 1004: return FLinearColor(1.00f, 0.82f, 0.02f); // Hazard Yellow
			case 1005: return FLinearColor(1.00f, 0.42f, 0.02f); // Safety Orange
			case 1006: return FLinearColor(0.78f, 0.10f, 1.00f); // Electric Violet
			case 1007: return FLinearColor(0.02f, 0.92f, 0.98f); // Laser Cyan
			case 1008: return FLinearColor(1.00f, 0.10f, 0.65f); // Hot Magenta
			default:
			{
				const uint8 HueByte = static_cast<uint8>(FMath::Abs(ID * 53) % 256);
				return FLinearColor::MakeFromHSV8(HueByte, 240, 255);
			}
		}
	}

	void ShowScreenMessage(int32 Key, float Duration, const FColor& Color, const FString& Text, float Scale)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(Key, Duration, Color, Text, true, FVector2D(Scale, Scale));
		}
	}

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
