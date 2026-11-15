// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Shared LiveWire utility helpers (ID color palette, on-screen messages, NavMesh queries).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UWorld;

namespace LiveWireUtils
{
	/** Maps any 4-digit LiveWire ID (1001..1008+) to a consistent saturated color. */
	FLinearColor GetColorForID(int32 ID);

	/** Displays a screen debug message when GEngine is available. */
	void ShowScreenMessage(int32 Key, float Duration, const FColor& Color, const FString& Text, float Scale = 1.0f);

	/** Projects WorldPos onto the NavMesh; returns false if unavailable or outside extent. */
	bool ProjectToNavMesh(const UWorld* World, const FVector& WorldPos, const FVector& Extent, FVector& OutLocation);

	/** Samples walkable ground Z via NavMesh projection, falling back to a downward static trace. */
	float SampleWalkableGroundZ(const UWorld* World, const FVector& WorldPos, const AActor* IgnoreActor = nullptr, float MinNormalZ = 0.5f);

	/** Casts Actor to T, or checks its attach parent if Actor is a child attachment. */
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
