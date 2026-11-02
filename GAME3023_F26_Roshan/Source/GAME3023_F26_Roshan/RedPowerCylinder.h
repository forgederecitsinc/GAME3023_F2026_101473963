// roshan - 101473963 - game3023 game engines 3
// red power cylinder actor header for live wire assignment
// this acts as a power cell that needs a battery inserted to turn on
// if hex code matches, cell powers ON. if mismatch, stays unpowered

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

// power cylinder cell actor
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

	// cylinder hex code - instance editable so i can place red, green, blue cells in level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Color Hex Code"))
	FString ColorHexCode = TEXT("FF0000");

	// runtime dynamic material for changing glow/dim state
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// updates material color from hex string
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromHex();

	// strips # and uppercases hex string
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FString GetNormalizedHexCode() const;

	// scene root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// cylinder mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CylinderMesh;

	// socket on top where the battery capsule snaps into
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	// trigger zone for pressing E
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// materials for on / off states (optional override)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> UnpoweredMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> PoweredMaterial;

	// power flags
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bIsPowered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bHasBattery = false;

	// pointer to the battery capsule plugged in right now
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
	// overlap events for prompts
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// updates glow depending on bIsPowered
	void UpdateVisualState();
};
