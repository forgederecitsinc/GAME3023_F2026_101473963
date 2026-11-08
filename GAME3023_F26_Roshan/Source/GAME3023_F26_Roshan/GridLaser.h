// roshan - 101473963 - game3023 game engines 3
// grid laser header - customizable security laser grid barrier / trap
// features instance-editable dimensions, beam counts, color, damage, and instant editor toggle (on/off)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLaser.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;
class AEnemyBatteryThief;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridLaserStateChanged, AGridLaser*, LaserGrid, bool, bIsActive);

UCLASS()
class GAME3023_F26_ROSHAN_API AGridLaser : public AActor
{
	GENERATED_BODY()
	
public:	
	AGridLaser();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// Core State & Toggles
	// ==========================================

	// Whether the laser grid is active. Instance editable so it can be set ON or OFF per placed actor in the level!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Is Laser On"))
	bool bIsLaserOn = true;

	// If true, the laser grid physically blocks the player like a solid energy barrier when active.
	// If false, it acts as a hazardous pass-through laser trap that burns the player.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Block Player Movement"))
	bool bBlockPlayer = false;

	// ==========================================
	// Grid Dimensions & Beam Layout
	// ==========================================

	// Width of the laser grid corridor / opening (along local Y axis)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridWidth = 400.0f;

	// Total height of the laser grid (along local Z axis)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridHeight = 300.0f;

	// Number of horizontal laser beams spanning the width
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumHorizontalBeams = 5;

	// Number of vertical laser beams spanning the height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumVerticalBeams = 4;

	// Toggle horizontal laser beams on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableHorizontalBeams = true;

	// Toggle vertical laser beams on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableVerticalBeams = true;

	// Cross-sectional thickness / diameter of each laser beam (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BeamThickness = 8.0f;

	// ==========================================
	// Laser Visuals & Materials
	// ==========================================

	// Emissive glow color of the laser beams
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	FLinearColor LaserColor = FLinearColor(1.0f, 0.05f, 0.05f, 1.0f);

	// Multiplier for laser emissive brightness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LaserEmissiveIntensity = 25.0f;

	// Laser beam material asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	TObjectPtr<UMaterialInterface> LaserMaterialAsset;

	// Frame / border material asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	TObjectPtr<UMaterialInterface> FrameMaterialAsset;

	// ==========================================
	// Frame / Emitter Posts
	// ==========================================

	// Whether to show the surrounding emitter frame posts (disable if mounting between custom level walls)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Frame")
	bool bShowEmitterPillars = true;

	// Thickness of the emitter frame posts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Frame", meta = (ClampMin = "5.0", UIMin = "5.0"))
	float PillarThickness = 20.0f;

	// ==========================================
	// Damage & Hazards
	// ==========================================

	// Damage dealt to the player's health when passing through or contacting active lasers (default 10 points)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageAmount = 10.0f;

	// Interval between damage ticks (in seconds) if the player remains inside the active laser grid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DamageTickInterval = 0.5f;

	// ==========================================
	// Blueprint & Runtime Controls
	// ==========================================

	// Turns the laser grid on or off at runtime
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void SetLaserState(bool bNewActive);

	// Toggles the laser grid state
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void ToggleLaser();

	// Checks if the laser is currently turned on
	UFUNCTION(BlueprintPure, Category = "LiveWire|GridLaser")
	bool IsLaserOn() const { return bIsLaserOn; }

	// Event broadcast whenever the laser grid state changes
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|GridLaser")
	FOnGridLaserStateChanged OnLaserStateChanged;

	// Rebuilds the geometry, instances, and frame dimensions
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void RebuildLaserGrid();

	// Updates visibility and collision based on bIsLaserOn
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserState();

	// Updates dynamic material parameters (color, glow intensity)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserVisuals();

protected:
	// Scene root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Instanced static mesh component for efficient batch rendering of all laser beams
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> LaserBeamInstances;

	// Hazard trigger / collision box for the laser field
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HazardBox;

	// Emitter frame meshes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LeftPillarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RightPillarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TopPillarMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BottomPillarMesh;

private:
	// Dynamic material instance for glowing laser beams
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserDynamicMat;

	// Currently overlapping player actor
	UPROPERTY(Transient)
	TObjectPtr<AGAME3023_F26_RoshanCharacter> OverlappingPlayer;

	// Timer tracking damage ticks while player stands in the laser grid
	float OverlapDamageTimer = 0.0f;

	// Applies laser damage to an actor
	void DealLaserDamage(AActor* TargetActor);

	// Hazard box overlap callbacks
	UFUNCTION()
	void OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
