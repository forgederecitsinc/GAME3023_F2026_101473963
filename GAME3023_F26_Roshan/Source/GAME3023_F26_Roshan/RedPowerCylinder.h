// roshan - 101473963 - game3023 game engines 3
// red power cylinder header
// when powered on with matching battery, lets out a thin laser beam
// laser length is instance editable so i can tune how far it reaches in the arena

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RedPowerCylinder.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class USceneComponent;
class UMaterialInterface;
class ABatteryCapsule;
class AGAME3023_F26_RoshanCharacter;

UCLASS()
class GAME3023_F26_ROSHAN_API ARedPowerCylinder : public AActor
{
	GENERATED_BODY()
	
public:	
	ARedPowerCylinder();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// 4-digit ID for this cell (default 1001)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Cell ID (4-Digit)"))
	int32 CellID = 1001;

	// optional manual color override
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Use Custom Color Override"))
	bool bUseCustomColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Custom Color", EditCondition = "bUseCustomColor"))
	FLinearColor CustomColor = FLinearColor::Red;

	// dynamic material for cylinder glow
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

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

	// cylinder mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CylinderMesh;

	// socket on top where the battery snaps in
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	// laser emitter socket on the front face of the cylinder
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> LaserEmitter;

	// thin visual laser beam mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	// laser length - instance editable per cylinder in the details panel!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Max Length", ClampMin = "50.0", UIMin = "50.0"))
	float LaserLength = 1200.0f;

	// laser beam thickness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Thickness", ClampMin = "1.0", UIMin = "1.0"))
	float LaserThickness = 6.0f;

	// stops the laser if it hits a wall or obstacle instead of going through
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Laser Stops At Obstacles"))
	bool bLaserStopsAtObstacles = true;

	// handy checkbox to preview laser reach in editor viewport without playing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Laser", meta = (DisplayName = "Preview Laser In Editor"))
	bool bPreviewLaserInEditor = false;

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
};
