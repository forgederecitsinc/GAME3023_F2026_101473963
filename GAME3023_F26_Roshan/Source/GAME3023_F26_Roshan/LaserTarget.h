// roshan - 101473963 - game3023 game engines 3
// laser target header - sharp-edged industrial receiver matching the substation arena blockout aesthetic
// detects incoming laser beams from RedPowerCylinder laser guns

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
	// Static Mesh Assets (Editable in BP_LaserTarget & Details panel)
	// ==========================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Base Pedestal Mesh"))
	TObjectPtr<UStaticMesh> BasePedestalMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Pillar Column Mesh"))
	TObjectPtr<UStaticMesh> PostColumnMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Neck Collar Mesh"))
	TObjectPtr<UStaticMesh> NeckCollarMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Target Body Mesh"))
	TObjectPtr<UStaticMesh> TargetBodyMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Top Cap Mesh"))
	TObjectPtr<UStaticMesh> TopCapMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Meshes", meta = (DisplayName = "Sensor Box Mesh"))
	TObjectPtr<UStaticMesh> SensorMeshAsset;

	// Optional base flat material used to create dynamic instances
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Base Material"))
	TObjectPtr<UMaterialInterface> BaseMaterial;

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

	// Automatically center the sensor box horizontally and vertically on the target body face
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Auto Center Sensor On Face"))
	bool bAutoCenterSensor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Sensor Box Location"))
	FVector SensorBoxLocation = FVector(0.0f, 98.0f, 185.0f);

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

	// Components - EditAnywhere & BlueprintReadWrite so they are fully editable in BP_LaserTarget
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// 1. Base pedestal slab
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePedestalMesh;

	// 2. Upright pillar column
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PostColumnMesh;

	// 3. Neck swivel platform collar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NeckCollarMesh;

	// 4. Main target receiver body box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TargetBodyMesh;

	// 5. Top cap box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TopCapMesh;

	// 6. Side sensor / receiver box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SensorMesh;
};
