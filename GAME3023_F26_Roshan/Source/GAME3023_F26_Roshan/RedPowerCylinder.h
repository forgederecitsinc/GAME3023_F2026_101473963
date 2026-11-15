// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: RedPowerCylinder.h
// Description: Industrial Sharp-Edged Laser Gun Turret (Red Power Cylinder).
// Designed as an industrial emitter turret. When the player inserts a Battery
// with the matching 4-digit ID (5-second hold interaction), the turret powers ON
// and shoots out a glowing focused laser beam from its front nozzle!
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedPowerCylinder.generated.h"

class USphereComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ABatteryCapsule;
class AGAME3023_F26_RoshanCharacter;
class ALaserTarget;
class ATransformerBushing;

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
	virtual bool ShouldTickIfViewportsOnly() const override;

	// 4-digit ID for this cell (default 1001)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Cell ID (4-Digit)"))
	int32 CellID = 1001;

	// Optional laser beam material override
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Laser Beam Material"))
	TObjectPtr<UMaterialInterface> LaserBeamMaterial;


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

	// socket where the battery snaps in when inserted
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	// laser emitter socket on the front tip of the nozzle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> LaserEmitter;

	// visual laser beam mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	// Blink interval in seconds for the powerDome green light when a battery is attached
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Blink Interval", ClampMin = "0.05", UIMin = "0.05"))
	float PowerDomeBlinkInterval = 0.5f;

	// Emissive multiplier for powerDome material when the green light is on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Glow Intensity", ClampMin = "1.0", UIMin = "1.0"))
	float PowerDomeGlowIntensity = 15.0f;

	// Point light intensity at powerDome when the green light is on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Light Intensity", ClampMin = "0.0", UIMin = "0.0"))
	float PowerDomeLightIntensity = 3000.0f;

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

	// rotates the gun head by given angle (default 90 deg clockwise)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
	void RotateGun(float AngleDegrees = 90.0f);

	// updates laser position, scale, and visibility
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
	void UpdateLaserBeam();

	// trigger zone for pressing E
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

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

	// Check whether player is close enough, on the cylinder's level (not under it), and has clear line of sight
	UFUNCTION(BlueprintPure, Category = "Cell|Actions")
	bool CanPlayerInteract(const AActor* InPlayerActor) const;

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
	void InitPowerDome();
	void ApplyPowerDomeLightState(bool bLightOn);

private:
	// one-shot print state latches for laser target detection
	bool bHasPrintedNoTarget = false;
	bool bHasPrintedTargetDetected = false;

	// tracked target hit for turning glow on/off
	UPROPERTY(Transient)
	TObjectPtr<ALaserTarget> LastHitLaserTarget;

	UPROPERTY(Transient)
	TObjectPtr<ATransformerBushing> LastHitBushing;

	// Cached Blueprint-added powerDome mesh and its dynamic materials / point light
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PowerDomeMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PowerDomeDynamicMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> PowerDomeLight;

	float PowerDomeBlinkTimer = 0.0f;
	bool bPowerDomeLightOn = false;

	// timer for periodic laser burn damage when player steps into the beam
	float PlayerLaserDamageTimer = 0.4f;
};
