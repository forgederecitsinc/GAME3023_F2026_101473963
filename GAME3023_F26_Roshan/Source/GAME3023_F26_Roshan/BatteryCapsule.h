#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveWireUtils.h"
#include "BatteryCapsule.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;

// A battery item that can be picked up, carried, dropped, or socketed into a power cylinder.
UCLASS()
class GAME3023_F26_ROSHAN_API ABatteryCapsule : public AActor
{
	GENERATED_BODY()

public:
	// Constructor: creates the battery mesh, collision, and interaction components.
	ABatteryCapsule();

	// Runs every frame to animate an idle battery or keep its carried state stable.
	virtual void Tick(float DeltaTime) override;

	ELiveWireShape BatteryShape = ELiveWireShape::Circle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	bool bEnableIdleAnimation = true;

	float RotationSpeed = 90.0f;

	float BatteryPickupRadius = 115.0f;

	float BobFrequency = 2.0f;

	float BobAmplitude = 10.0f;

	bool bIsCarried = false;

	bool bIsSocketed = false;

	FVector SpawnOrigin;

	// Allows the player to collect this battery.
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	// Marks the battery as carried and disables world pickup collision.
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnCollected(AGAME3023_F26_RoshanCharacter* Player);

	// Attaches the battery to a power cylinder socket.
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnSocketed(AActor* SocketActor, USceneComponent* AttachTargetComp);

	// Drops the battery back into the world at a target location.
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnDropped(const FVector& DropLocation);

protected:
	// Runs when the battery enters the game.
	virtual void BeginPlay() override;

	// Updates the battery while it is placed or changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	// Refreshes the battery after an editor property changes.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Runs when the player enters the battery pickup area.
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Runs when the player leaves the battery pickup area.
	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// Enables or disables collision while the battery is attached or carried.
	void SetAttachmentCollisionEnabled(bool bEnabled);

	// Moves the battery to a world location after it is dropped.
	void ResetToWorldLocation(const FVector& TargetLocation);

	float RunningTime = 0.0f;
	FVector BaseLocation;
};
