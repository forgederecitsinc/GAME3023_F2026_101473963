// Fill out your copyright notice in the Description page of Project Settings.

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

/**
 * Red Power Cylinder (Cell) for the "Live Wire" circuit puzzle.
 * - Requires a BatteryCapsule inserted to power on.
 * - Sockets the battery visually onto its top slot.
 * - Prints debug messages showing its power state.
 */
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

	// Instance-editable Color Hex Code (e.g. #FF0000 for Red, #00FF00 for Green, #0000FF for Blue)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Color Hex Code"))
	FString ColorHexCode = TEXT("FF0000");

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromHex();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FString GetNormalizedHexCode() const;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CylinderMesh;

	// Attachment socket on top of cylinder where battery locks in
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> BatterySocket;

	// Proximity trigger for interacting
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// Visual Materials
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> UnpoweredMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cell|Materials")
	TObjectPtr<UMaterialInterface> PoweredMaterial;

	// Power State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bIsPowered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	bool bHasBattery = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cell|State")
	TObjectPtr<ABatteryCapsule> InsertedBattery;

	// Actions
	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	bool InsertBattery(ABatteryCapsule* Battery);

	UFUNCTION(BlueprintCallable, Category = "Cell|Actions")
	ABatteryCapsule* RemoveBattery();

	UFUNCTION(BlueprintPure, Category = "Cell|State")
	bool IsPowered() const { return bIsPowered; }

protected:
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void UpdateVisualState();
};
