#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UWorld;

namespace LiveWireUtils
{
	// Returns a fixed color for known LiveWire IDs, or a generated color for new IDs.
	FLinearColor GetColorForID(int32 ID);

	// Shows a temporary debug message on the game screen.
	void ShowScreenMessage(int32 Key, float Duration, const FColor& Color, const FString& Text, float Scale = 1.0f);

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
