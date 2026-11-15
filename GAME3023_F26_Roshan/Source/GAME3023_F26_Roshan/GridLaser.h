// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GridLaser.h
// Description: Security Grid Laser Barrier / Hazard System.
// Creates a glowing sci-fi laser field (like in spy/heist movies) with customizable
// horizontal and vertical beams.
// =============================================================================

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
	// Constructor: sets up components and default materials
	AGridLaser();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// Live editor preview: immediately updates beams when you tweak properties in Details panel!
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 🎮 CORE TOGGLES (Change these per actor in the level!)
	// =========================================================================

	// ✏️ CHANGE THIS: Toggle this specific laser barrier ON or OFF!
	// You can check/uncheck this right in the Unreal Editor Details panel to turn it on/off!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Is Laser On"))
	bool bIsLaserOn = true;

	// ✏️ CHANGE THIS: 
	// - If TRUE: acts as an impassable solid forcefield (physically blocks player from walking through)
	// - If FALSE: acts as a hazardous walk-through laser trap that burns the player for damage!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Block Player Movement"))
	bool bBlockPlayer = false;

	// =========================================================================
	// 📐 GRID DIMENSIONS & BEAM COUNTS (Shape of the doorway / barrier)
	// =========================================================================

	// ✏️ CHANGE THIS: Total width of the doorway/opening along local Y axis (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridWidth = 400.0f;

	// ✏️ CHANGE THIS: Total height of the doorway/opening along local Z axis (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridHeight = 300.0f;

	// ✏️ CHANGE THIS: How many horizontal laser beams to draw (across the doorway)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumHorizontalBeams = 5;

	// ✏️ CHANGE THIS: How many vertical laser beams to draw (from top to bottom)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumVerticalBeams = 4;

	// Toggle horizontal laser beams on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableHorizontalBeams = true;

	// Toggle vertical laser beams on/off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableVerticalBeams = true;

	// ✏️ CHANGE THIS: Thickness / diameter of each glowing laser cylinder/beam (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BeamThickness = 8.0f;

	// =========================================================================
	// 🎨 VISUALS & MATERIALS (Colors and Glow)
	// =========================================================================

	// ✏️ CHANGE THIS: Emissive glow color of the laser beams (default is bright red)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	FLinearColor LaserColor = FLinearColor(1.0f, 0.05f, 0.05f, 1.0f);

	// ✏️ CHANGE THIS: Glow intensity multiplier (higher values = more bloom in Unreal Engine post-process)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LaserEmissiveIntensity = 25.0f;

	// Base laser material asset (uses M_laserbeam)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	TObjectPtr<UMaterialInterface> LaserMaterialAsset;

	// =========================================================================
	// 💥 DAMAGE & HAZARD SETTINGS
	// =========================================================================

	// ✏️ CHANGE THIS: Damage dealt to the player's health when touching active lasers
	// (Default is 10.0 HP per hit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageAmount = 10.0f;

	// ✏️ CHANGE THIS: Time interval between continuous damage ticks if player stays inside the lasers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DamageTickInterval = 0.5f;

	// =========================================================================
	// 🧠 BLUEPRINT & RUNTIME FUNCTIONS
	// =========================================================================

	// Call this from Blueprint (e.g. from a button or trigger) to turn lasers on/off
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void SetLaserState(bool bNewActive);

	// Toggles the laser state (turns on if off, turns off if on)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void ToggleLaser();

	// Returns true if the laser is currently turned on
	UFUNCTION(BlueprintPure, Category = "LiveWire|GridLaser")
	bool IsLaserOn() const { return bIsLaserOn; }

	// Event broadcast when the laser turns on or off
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|GridLaser")
	FOnGridLaserStateChanged OnLaserStateChanged;

	// Rebuilds the mesh geometry, instances, and frame pillars based on current settings
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void RebuildLaserGrid();

	// Updates visibility and collision based on bIsLaserOn
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserState();

	// Updates dynamic material parameters (color, glow intensity)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserVisuals();

protected:
	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Instanced static mesh component for efficient batch rendering of all laser beams
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> LaserBeamInstances;

	// Hazard trigger / collision box for the laser field
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HazardBox;

private:
	// Dynamic material instance for glowing laser beams
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserDynamicMat;

	// Pointer to player character currently inside the laser hazard box
	UPROPERTY(Transient)
	TObjectPtr<AGAME3023_F26_RoshanCharacter> OverlappingPlayer;

	// Timer for periodic damage ticks while standing inside the laser
	float OverlapDamageTimer = 0.0f;

	// Helper function that applies damage to player or kills Battery Thief permanently
	void DealLaserDamage(AActor* TargetActor);

	// Overlap event callbacks
	UFUNCTION()
	void OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
