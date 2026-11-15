// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: LaserTarget.h
// Description: Laser Receiver Target actor.
// Detects incoming laser beams from RedPowerCylinder laser turrets.
// Matches the substation arena blockout aesthetic and fires events to light up
// matching Transformer Bushings or trigger doors!
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserTarget.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserTargetHitStateChanged, ALaserTarget*, Target, bool, bIsHit);

UCLASS()
class GAME3023_F26_ROSHAN_API ALaserTarget : public AActor
{
	GENERATED_BODY()
	
public:	
	ALaserTarget();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Target ID for matching with laser cell IDs (1001 = Red)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Target ID (4-Digit)"))
	int32 TargetID = 1001;

	// ==========================================
	// Components
	// ==========================================

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Laser hit state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Hit By Laser"))
	bool bIsHitByLaser = false;

	// Event fired when laser hit state changes
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnLaserTargetHitStateChanged OnLaserHitStateChanged;

	// Set whether target is being hit by a laser
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	// Updates material colors (materials are set in Blueprint, not overwritten in C++)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColors();

	// Converts any 4 digit ID into a consistent color palette matching RedPowerCylinder
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;
};
