// roshan - 101473963 - game3023 game engines 3
// battery capsule actor header for live wire project
// player can grab this, socket it into the power cylinder, or drop it with R

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BatteryCapsule.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UStaticMeshComponent;
class AGAME3023_F26_RoshanCharacter;

// battery cell pickup actor
// prof said only 1 battery in inventory at once
UCLASS()
class GAME3023_F26_ROSHAN_API ABatteryCapsule : public AActor
{
	GENERATED_BODY()
	
public:	
	ABatteryCapsule();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:	
	virtual void Tick(float DeltaTime) override;

	// hex code for color - default red ff0000
	// need this editable in editor so we can test different color batteries
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Color Hex Code"))
	FString ColorHexCode = TEXT("FF0000");

	// dynamic mat instance to change color at runtime
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// helper to parse the hex and apply to material
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromHex();

	// strips # and makes uppercase just in case i typed lowercase in editor
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FString GetNormalizedHexCode() const;

	// collision capsule
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// the actual cylinder mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BatteryMesh;

	// overlap sphere for pressing E
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// floating animation settings so it looks like a real video game pickup
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	bool bEnableIdleAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobFrequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobAmplitude = 10.0f;

	// state flags - keeping track of where this thing is
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsCarried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsSocketed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	FVector SpawnOrigin;

	// pickup and socket functions
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnCollected(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnSocketed(AActor* SocketActor, USceneComponent* AttachTargetComp);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnDropped(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void RespawnToOrigin();

protected:
	// overlap events for E prompt
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// timer for bob math
	float RunningTime = 0.0f;
	FVector BaseLocation;
};
