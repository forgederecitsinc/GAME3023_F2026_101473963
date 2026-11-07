// roshan - 101473963 - game3023 game engines 3
// red power cylinder header - redesigned as an industrial sharp-edged laser gun turret
// shoots out a bright focused laser beam from the front nozzle once matching battery is attached
// sharp-edged box geometry throughout to match the substation arena blockout aesthetic

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedPowerCylinder.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USceneComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ABatteryCapsule;
class AGAME3023_F26_RoshanCharacter;
class ALaserTarget;

UCLASS()
class GAME3023_F26_ROSHAN_API ARedPowerCylinder : public AActor
{
	GENERATED_BODY()
	
public:	
	ARedPowerCylinder();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// 4-digit ID for this cell (default 1001)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Cell ID (4-Digit)"))
	int32 CellID = 1001;

	// Toggle to automatically set primary colors (Body, Barrel, Dock, Laser) based on Cell ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Sync Colors From Cell ID"))
	bool bSyncColorsFromCellID = false;

	// --- Instance Editable Mesh Colors (Customizable in Details Panel per Instance) ---

	// Main cylinder gun body mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Main Body Color (Cylinder)"))
	FLinearColor BodyColor = FLinearColor(1.0f, 0.10f, 0.12f);

	// Front barrel mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Barrel Color"))
	FLinearColor BarrelColor = FLinearColor(1.0f, 0.10f, 0.12f);

	// Front emitter nozzle tip mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Nozzle Tip Color"))
	FLinearColor NozzleColor = FLinearColor(0.08f, 0.08f, 0.10f);

	// Battery dock cradle mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Battery Dock Color"))
	FLinearColor BatteryDockColor = FLinearColor(0.75f, 0.08f, 0.10f);

	// Neck collar plate mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Neck Collar Color"))
	FLinearColor NeckCollarColor = FLinearColor(0.14f, 0.14f, 0.16f);

	// Upright pillar post column mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Pillar Post Color"))
	FLinearColor PillarColor = FLinearColor(0.80f, 0.82f, 0.86f);

	// Base pedestal middle tier slab mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Base Pedestal Color"))
	FLinearColor PedestalColor = FLinearColor(0.82f, 0.82f, 0.85f);

	// Base bottom foundation plinth slab mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Base Plinth Color"))
	FLinearColor PlinthColor = FLinearColor(0.12f, 0.12f, 0.14f);

	// Laser beam mesh color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Mesh Colors", meta = (DisplayName = "Laser Beam Color"))
	FLinearColor LaserBeamColor = FLinearColor(1.0f, 0.10f, 0.12f);

	// Legacy backward-compatibility aliases
	UPROPERTY()
	bool bUseCustomColor = false;

	UPROPERTY()
	FLinearColor CustomColor = FLinearColor(1.0f, 0.10f, 0.12f);

	UPROPERTY()
	bool bTintBatteryDock = true;

	// dynamic materials for laser gun glow
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BarrelDynamicMaterial;

	// laser dynamic material instance so the beam glows super bright
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserDynamicMaterial;

	// updates material color based on ID
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromID();

	// converts any 4 digit id into a consistent color
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// scene root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// base bottom plinth - sharp dark square slab on floor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePlinthMesh;

	// base pedestal plate - sharp middle tier slab
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePedestalMesh;

	// upright pillar post - sharp square column
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PostColumnMesh;

	// neck collar - square plate beneath the gun head
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NeckCollarMesh;

	// main gun receiver body - sharp rectangular box tinted with cell color
	// kept named CylinderMesh so existing code and blueprints continue working cleanly
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CylinderMesh;

	// front stepped barrel extension - sharp rectangular box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BarrelMesh;

	// front emitter nozzle tip - sharp dark box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> NozzleMesh;

	// battery cradle socket bracket on top deck of the gun
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BatteryDockMesh;

	// socket where the battery snaps in when inserted
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	// laser emitter socket on the front tip of the nozzle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> LaserEmitter;

	// visual laser beam mesh - sharp rectangular laser beam
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	// laser length - instance editable per cylinder in the details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Max Length", ClampMin = "50.0", UIMin = "50.0"))
	float LaserLength = 2500.0f;

	// laser beam thickness (sharp square cross-section)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Thickness", ClampMin = "1.0", UIMin = "1.0"))
	float LaserThickness = 14.0f;

	// stops the laser if it hits a wall or obstacle instead of going through
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Stops At Obstacles"))
	bool bLaserStopsAtObstacles = true;

	// gun aim yaw angle in degrees (0, 90, 180, 270)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Gun Aim Yaw"))
	float GunYaw = 0.0f;

	// handy checkbox to preview laser reach in editor viewport without playing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Preview Laser In Editor"))
	bool bPreviewLaserInEditor = false;

	// Whether the laser beam is currently hitting an object with matching Target ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Laser", meta = (DisplayName = "Is Matching Target Detected"))
	bool bIsTargetDetected = false;

	// Currently detected matching actor (displayed in Details panel during play)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Laser", meta = (DisplayName = "Detected Target"))
	TObjectPtr<AActor> DetectedTarget;

	// Checks if an actor has a Target ID matching this cylinder's CellID
	UFUNCTION(BlueprintPure, Category = "LiveWire|Laser")
	bool CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const;

	// whether to show the bottom base plinth and pedestal slab (disable if placing on an existing arena pedestal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Show Base Plinth & Pedestal"))
	bool bShowBaseStructure = true;

	// whether the upright pillar post uses a round cylinder (matches arena turrets in the level) or a sharp square box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Use Cylinder Post (Arena Style)"))
	bool bUseCylinderPost = true;

	// rotates the gun head by given angle (default 90 deg clockwise)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
	void RotateGun(float AngleDegrees = 90.0f);

	// updates laser position, scale, and visibility
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
	void UpdateLaserBeam();

	// trigger zone for pressing E
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// optional material overrides
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> UnpoweredMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> PoweredMaterial;

	// power flags
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bIsPowered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bHasBattery = false;

	// currently attached battery
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	TObjectPtr<ABatteryCapsule> InsertedBattery;

	// interaction logic
	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool InsertBattery(ABatteryCapsule* Battery);

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	ABatteryCapsule* RemoveBattery();

	UFUNCTION(BlueprintPure, Category = "Cell|State")
	bool IsPowered() const { return bIsPowered; }

protected:
	// overlap events
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateVisualState();
	void BuildGunComponents();

private:
	// one-shot print state latches for laser target detection
	bool bHasPrintedNoTarget = false;
	bool bHasPrintedTargetDetected = false;

	// tracked target hit for turning glow on/off
	UPROPERTY(Transient)
	TObjectPtr<ALaserTarget> LastHitLaserTarget;
};
