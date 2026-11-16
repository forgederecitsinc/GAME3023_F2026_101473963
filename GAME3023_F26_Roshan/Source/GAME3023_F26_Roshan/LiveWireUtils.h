#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UWorld;

namespace LiveWireUtils
{
	/** Maps any 4-digit LiveWire ID (1001..1008+) to a consistent saturated color. */
 // ID color mapping and dynamic materials
	FLinearColor GetColorForID(int32 ID);

	/** Displays a screen debug message when GEngine is available. */
 // ShowScreenMessage: show screen message
	void ShowScreenMessage(int32 Key, float Duration, const FColor& Color, const FString& Text, float Scale = 1.0f);

	/** Projects WorldPos onto the NavMesh; returns false if unavailable or outside extent. */
 // Navigation and pathfinding
	bool ProjectToNavMesh(const UWorld* World, const FVector& WorldPos, const FVector& Extent, FVector& OutLocation);

	/** Samples walkable ground Z via NavMesh projection, falling back to a downward static trace. */
 // SampleWalkableGroundZ: sample walkable ground z
	float SampleWalkableGroundZ(const UWorld* World, const FVector& WorldPos, const AActor* IgnoreActor = nullptr, float MinNormalZ = 0.5f);

	/** Casts Actor to T, or checks its attach parent if Actor is a child attachment. */
	template <typename T>
 // ResolveActorOrParent: resolve actor or parent
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
