#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserTarget.generated.h"

class USceneComponent;
class ATransformerBushing;

// Delegate that lets Blueprints know when this target is hit or stops being hit.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserTargetHitStateChanged, ALaserTarget*, Target, bool, bIsHit);

// Actor used as a laser target.
UCLASS()
class GAME3023_F26_ROSHAN_API ALaserTarget : public AActor
{
	GENERATED_BODY()

public:
	// Constructor: creates the target and its root component.
	ALaserTarget();

	// Optional bushing to light up when this target is hit by a laser.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Linked Bushing (Optional)"))
	TObjectPtr<ATransformerBushing> LinkedBushing;

	// Root component used as the base transform of the target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Stores whether the laser is currently hitting this target.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Hit By Laser"))
	bool bIsHitByLaser = false;

	// Blueprint event broadcast after the laser hit state changes.
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnLaserTargetHitStateChanged OnLaserHitStateChanged;

	// Changes whether this target is currently hit and broadcasts the result.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);
};
