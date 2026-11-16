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

 // Called every frame
	virtual void Tick(float DeltaTime) override;
 // Called every frame
	virtual bool ShouldTickIfViewportsOnly() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Cell ID (4-Digit)"))
	int32 CellID = 1001;

	UPROPERTY()
	bool bUseCustomColor = false;

	UPROPERTY()
	FLinearColor CustomColor = FLinearColor(1.0f, 0.10f, 0.12f);

	UPROPERTY()
	bool bTintBatteryDock = true;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BarrelDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserDynamicMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> LaserEmitter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Blink Interval", ClampMin = "0.05", UIMin = "0.05"))
	float PowerDomeBlinkInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Glow Intensity", ClampMin = "1.0", UIMin = "1.0"))
	float PowerDomeGlowIntensity = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Visuals", meta = (DisplayName = "Power Dome Light Intensity", ClampMin = "0.0", UIMin = "0.0"))
	float PowerDomeLightIntensity = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Max Length", ClampMin = "50.0", UIMin = "50.0"))
	float LaserLength = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Thickness", ClampMin = "1.0", UIMin = "1.0"))
	float LaserThickness = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Stops At Obstacles"))
	bool bLaserStopsAtObstacles = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Gun Aim Yaw"))
	float GunYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Preview Laser In Editor"))
	bool bPreviewLaserInEditor = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Laser", meta = (DisplayName = "Is Matching Target Detected"))
	bool bIsTargetDetected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Laser", meta = (DisplayName = "Detected Target"))
	TObjectPtr<AActor> DetectedTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bIsPowered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bHasBattery = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	TObjectPtr<ABatteryCapsule> InsertedBattery;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
 // ID color mapping and dynamic materials
	void ApplyColorFromID();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
 // ID color mapping and dynamic materials
	FLinearColor GetColorForID(int32 InID) const;

	UFUNCTION(BlueprintPure, Category = "LiveWire|Laser")
 // CheckActorTargetIDMatch: check actor target id match
	bool CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
 // RotateGun: rotate gun
	void RotateGun(float AngleDegrees = 90.0f);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
 // Laser trace and collision
	void UpdateLaserBeam();

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
 // Handles Interact interaction
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintPure, Category = "Cell|Actions")
 // Handles CanPlayerInteract interaction
	bool CanPlayerInteract(const AActor* InPlayerActor) const;

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
 // Battery state and handling
	bool InsertBattery(ABatteryCapsule* Battery);

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
 // Battery state and handling
	ABatteryCapsule* RemoveBattery();

	UFUNCTION(BlueprintPure, Category = "Cell|State")
 // IsPowered: is powered
	bool IsPowered() const { return bIsPowered; }

protected:
 // Called when the game starts or when spawned
	virtual void BeginPlay() override;
 // Editor construction script
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
 // Editor property change callback
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
 // Handles OnInteractionBeginOverlap interaction
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
 // Handles OnInteractionEndOverlap interaction
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

 // UpdateVisualState: update visual state
	void UpdateVisualState();
 // BuildGunComponents: build gun components
	void BuildGunComponents();
 // InitPowerDome: init power dome
	void InitPowerDome();
 // ApplyPowerDomeLightState: apply power dome light state
	void ApplyPowerDomeLightState(bool bLightOn);

private:
 // ClearDetectedTargets: clear detected targets
	void ClearDetectedTargets();
 // Laser trace and collision
	AActor* TraceLaserBeam(float& OutDistance, bool& bOutHitObstacle) const;
 // Laser trace and collision
	void ApplyLaserBurnToPlayer(AActor* HitActor);
 // UpdateDetectedTargets: update detected targets
	void UpdateDetectedTargets(AActor* HitActor);

	bool bHasPrintedNoTarget = false;
	bool bHasPrintedTargetDetected = false;

	UPROPERTY(Transient)
	TObjectPtr<ALaserTarget> LastHitLaserTarget;

	UPROPERTY(Transient)
	TObjectPtr<ATransformerBushing> LastHitBushing;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> PowerDomeMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> PowerDomeOriginalMaterials;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PowerDomeGreenDynMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PowerDomeRedDynMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> PowerDomeLight;

	float PowerDomeBlinkTimer = 0.0f;
	bool bPowerDomeLightOn = false;
	float PlayerLaserDamageTimer = 0.4f;
};
