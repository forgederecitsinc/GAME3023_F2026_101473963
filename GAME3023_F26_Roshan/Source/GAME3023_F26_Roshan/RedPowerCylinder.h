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
	// Constructor: creates the cylinder, battery socket, laser emitter, and interaction zone.
	ARedPowerCylinder();

	// Runs every frame to update the laser and power dome.
	virtual void Tick(float DeltaTime) override;

	// Allows laser preview updates to run in editor viewports.
	virtual bool ShouldTickIfViewportsOnly() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Cell ID (4-Digit)"))
	int32 CellID = 1001;

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

	// Returns the mapped color for a four-digit ID.
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// Checks whether an actor has an ID that matches this cylinder.
	UFUNCTION(BlueprintPure, Category = "LiveWire|Laser")
	bool CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const;

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
