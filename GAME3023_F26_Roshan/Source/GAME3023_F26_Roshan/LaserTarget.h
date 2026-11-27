#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserTarget.generated.h"

class USceneComponent;

// Delegate that lets Blueprints know when this target is hit or stops being hit.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserTargetHitStateChanged, ALaserTarget*, Target, bool, bIsHit);

// Actor used as a laser target and identified by a four-digit ID.
UCLASS()
class GAME3023_F26_ROSHAN_API ALaserTarget : public AActor
{
	GENERATED_BODY()

public:
	// Constructor: creates the target and its root component.
	ALaserTarget();

	// Four-digit ID used to identify this target and choose its color.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Target ID (4-Digit)"))
	int32 TargetID = 1001;

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

	// Returns the color that should be used for a specific target ID.
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;
};
