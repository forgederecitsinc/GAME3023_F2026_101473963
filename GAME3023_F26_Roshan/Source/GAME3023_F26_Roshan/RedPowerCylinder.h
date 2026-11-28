#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveWireUtils.h"
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
	// Constructor: creates the cylinder, battery socket, laser emitter, and interaction zone.
	ARedPowerCylinder();

	// Runs every frame to update the laser and power dome.
	virtual void Tick(float DeltaTime) override;

	// Allows laser preview updates to run in editor viewports.
	virtual bool ShouldTickIfViewportsOnly() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Shape", meta = (DisplayName = "Required Battery Shape"))
	ELiveWireShape RequiredShape = ELiveWireShape::Cube;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LaserEmitter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	float PowerDomeBlinkInterval = 0.5f;

	float PowerDomeGlowIntensity = 15.0f;

	float PowerDomeLightIntensity = 3000.0f;

	float LaserLength = 2500.0f;

	float LaserThickness = 14.0f;

	bool bLaserStopsAtObstacles = true;

	bool bPreviewLaserInEditor = false;

	bool bIsTargetDetected = false;

	UPROPERTY()
	TObjectPtr<AActor> DetectedTarget;

	bool bIsPowered = false;

	bool bHasBattery = false;

	UPROPERTY()
	TObjectPtr<ABatteryCapsule> InsertedBattery;

	// Updates the laser beam, target hit state, and player damage.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Laser")
	void UpdateLaserBeam();

	// Handles inserting or removing a battery when the player interacts.
	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	// Checks whether the player is close enough and has line of sight.
	UFUNCTION(BlueprintPure, Category = "Cell|Actions")
	bool CanPlayerInteract(const AActor* InPlayerActor) const;

	// Inserts a battery capsule and returns whether its ID matches.
	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool InsertBattery(ABatteryCapsule* Battery);

	// Removes the inserted battery and returns it to the player.
	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	ABatteryCapsule* RemoveBattery();

	// Returns true if the cylinder has a matching powered battery.
	UFUNCTION(BlueprintPure, Category = "Cell|State")
	bool IsPowered() const { return bIsPowered; }

protected:
	// Runs when the game starts or the cylinder is spawned.
	virtual void BeginPlay() override;

	// Updates cylinder visuals while it is changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	// Updates the cylinder after an editor property is changed.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Runs when a player enters the interaction zone.
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Runs when a player leaves the interaction zone.
	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Finds and prepares the power dome mesh and blinking light.
	void InitPowerDome();

	// Applies green, red, or inactive power dome visuals.
	void ApplyPowerDomeLightState(bool bLightOn);

private:
	// Creates a dynamic emissive material for the power dome.
	UMaterialInstanceDynamic* CreatePowerDomeMaterial(const FLinearColor& BaseColor);

	// Clears the currently detected target and laser hit state.
	void ClearDetectedTargets();

	// Fires a line trace and returns the first hit actor.
	AActor* TraceLaserBeam(float& OutDistance, bool& bOutHitObstacle) const;

	// Damages the player if the laser beam hits them.
	void ApplyLaserBurnToPlayer(AActor* HitActor);

	// Updates target actors that match the cylinder ID.
	void UpdateDetectedTargets(AActor* HitActor);

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
