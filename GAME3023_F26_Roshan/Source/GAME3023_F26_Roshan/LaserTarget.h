// roshan - 101473963 - game3023 game engines 3
// laser target header - sharp-edged industrial receiver matching the substation arena blockout aesthetic
// detects incoming laser beams from RedPowerCylinder laser guns

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ALaserTarget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLaserTargetHitStateChanged, ALaserTarget*, HitTarget, bool, bIsHit);

#include "LaserTarget.generated.h"

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

	// Optional manual color override for the main target body
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Use Custom Color Override"))
	bool bUseCustomColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Custom Color", EditCondition = "bUseCustomColor"))
	FLinearColor CustomColor = FLinearColor(1.0f, 0.10f, 0.12f);

	// Base pedestal slab color (clean light industrial concrete/cream)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Base Pedestal Color"))
	FLinearColor PedestalColor = FLinearColor(0.82f, 0.82f, 0.85f);

	// Pillar post column color (silver metallic alloy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Pillar Post Color"))
	FLinearColor PillarColor = FLinearColor(0.80f, 0.82f, 0.86f);

	// Neck collar plate color (dark industrial charcoal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Neck Collar Color"))
	FLinearColor NeckCollarColor = FLinearColor(0.14f, 0.14f, 0.16f);

	// Top cap box color (dark industrial charcoal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Top Cap Color"))
	FLinearColor TopCapColor = FLinearColor(0.12f, 0.12f, 0.14f);

	// Side sensor receiver box color (dark charcoal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Sensor Box Color"))
	FLinearColor SensorColor = FLinearColor(0.10f, 0.10f, 0.12f);

	// Whether to show the bottom base pedestal slab (disable if placing on an existing arena pedestal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Show Base Structure"))
	bool bShowBaseStructure = true;

	// Proportions and offsets for fine-tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Base Pedestal Scale"))
	FVector BasePedestalScale = FVector(2.40f, 2.40f, 0.40f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Pillar Scale"))
	FVector PillarScale = FVector(0.85f, 0.85f, 1.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Neck Collar Scale"))
	FVector NeckCollarScale = FVector(1.90f, 1.90f, 0.20f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Target Body Scale"))
	FVector TargetBodyScale = FVector(1.60f, 1.60f, 0.90f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Top Cap Scale"))
	FVector TopCapScale = FVector(0.75f, 0.75f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Sensor Box Scale"))
	FVector SensorBoxScale = FVector(0.50f, 0.40f, 0.45f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Sensor Box Location"))
	FVector SensorBoxLocation = FVector(55.0f, 98.0f, 185.0f);

	// Laser hit state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Hit By Laser"))
	bool bIsHitByLaser = false;

	// Whether the target body and sensor glow when struck by laser
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Glow When Hit"))
	bool bGlowWhenHit = true;

	// Dynamic materials for runtime color and glow updates
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PedestalDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PillarDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CollarDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CapDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SensorDynamicMaterial;

	// Event fired when laser hit state changes
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnLaserTargetHitStateChanged OnLaserHitStateChanged;

	// Set whether target is being hit by a laser
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	// Updates material colors across all components
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColors();

	// Converts any 4 digit ID into a consistent color palette matching RedPowerCylinder
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 1. Base pedestal slab
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePedestalMesh;

	// 2. Upright pillar column
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PostColumnMesh;

	// 3. Neck swivel platform collar
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NeckCollarMesh;

	// 4. Main target receiver body box
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TargetBodyMesh;

	// 5. Top cap box
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TopCapMesh;

	// 6. Side sensor / receiver box
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SensorMesh;
};
