#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveWireUtils.generated.h"

class UWorld;

// Shapes used to visually match batteries with their corresponding laser guns.
UENUM(BlueprintType)
enum class ELiveWireShape : uint8
{
	Cube		UMETA(DisplayName = "Cube"),
	Sphere		UMETA(DisplayName = "Sphere"),
	Cylinder	UMETA(DisplayName = "Cylinder"),
	Cone		UMETA(DisplayName = "Cone")
};

namespace LiveWireUtils
{
	// Finds the nearest valid NavMesh location near a world position.
	bool ProjectToNavMesh(const UWorld* World, const FVector& WorldPos, const FVector& Extent, FVector& OutLocation);

	// Finds a walkable floor height using the NavMesh or a downward line trace.
	float SampleWalkableGroundZ(const UWorld* World, const FVector& WorldPos, const AActor* IgnoreActor = nullptr, float MinNormalZ = 0.5f);

	// Returns the actor directly as type T, or checks its parent actor if it is attached.
	template <typename T>
	T* ResolveActorOrParent(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (T* Direct = Cast<T>(Actor))
		{
			return Direct;
		}
		return Cast<T>(Actor->GetAttachParentActor());
	}
}
