// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Laser Receiver Target: detects laser hits and notifies matching Transformer Bushings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserTarget.generated.h"

class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserTargetHitStateChanged, ALaserTarget*, Target, bool, bIsHit);

UCLASS()
class GAME3023_F26_ROSHAN_API ALaserTarget : public AActor
{
	GENERATED_BODY()

public:
	ALaserTarget();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Target ID (4-Digit)"))
	int32 TargetID = 1001;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Hit By Laser"))
	bool bIsHitByLaser = false;

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnLaserTargetHitStateChanged OnLaserHitStateChanged;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColors();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;
};
