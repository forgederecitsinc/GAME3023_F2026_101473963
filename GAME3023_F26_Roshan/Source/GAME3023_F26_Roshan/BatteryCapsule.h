// roshan - 101473963 - game3023 game engines 3
// battery capsule header
// changed from hex string to 4-digit id because hex was annoying to type in editor
// battery id has to match the cylinder cell id to power it on

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BatteryCapsule.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UStaticMeshComponent;
class AGAME3023_F26_RoshanCharacter;

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

	// 4-digit ID for this battery (default 1001)
	// can type whatever 4-digit number in the details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Battery ID (4-Digit)"))
	int32 BatteryID = 1001;

	// optional checkbox if prof wants us to manually pick a color
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Use Custom Color Override"))
	bool bUseCustomColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Color", meta = (DisplayName = "Custom Color", EditCondition = "bUseCustomColor"))
	FLinearColor CustomColor = FLinearColor::Red;

	// runtime dynamic material instance
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// updates material color based on ID
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromID();

	// converts any 4 digit id into a consistent color
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// collision capsule
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// mesh component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BatteryMesh;

	// interaction trigger zone
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// idle bobbing and spin settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	bool bEnableIdleAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobFrequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobAmplitude = 10.0f;

	// state flags
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsCarried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsSocketed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	FVector SpawnOrigin;

	// interaction functions
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
	// overlap events
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	float RunningTime = 0.0f;
	FVector BaseLocation;
};
