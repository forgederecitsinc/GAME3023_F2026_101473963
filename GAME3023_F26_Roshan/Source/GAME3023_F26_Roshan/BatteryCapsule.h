#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BatteryCapsule.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;

UCLASS()
class GAME3023_F26_ROSHAN_API ABatteryCapsule : public AActor
{
	GENERATED_BODY()

public:
	ABatteryCapsule();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Battery ID (4-Digit)"))
	int32 BatteryID = 1001;

	UPROPERTY()
	bool bUseCustomColor = false;

	UPROPERTY()
	FLinearColor CustomColor = FLinearColor(0.0000f, 0.8714f, 1.0000f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	bool bEnableIdleAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobFrequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobAmplitude = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsCarried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsSocketed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	FVector SpawnOrigin;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromID();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

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
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	void SetAttachmentCollisionEnabled(bool bEnabled);
	void ResetToWorldLocation(const FVector& TargetLocation);

	float RunningTime = 0.0f;
	FVector BaseLocation;
};
