#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneLaserParticle.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

// Falling laser particle dropped by an enemy drone.
UCLASS()
class GAME3023_F26_ROSHAN_API ADroneLaserParticle : public AActor
{
	GENERATED_BODY()

public:
	// Constructor: creates the collision sphere, mesh, and light.
	ADroneLaserParticle();

	// Moves the particle downward every frame.
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ParticleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> ParticleLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float DamageAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float FallSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FVector MoveDirection = FVector(0.0f, 0.0f, -1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FLinearColor ParticleColor = FLinearColor(1.0f, 0.08f, 0.08f);

	// Sets the particle damage, speed, direction, and color.
	void InitializeParticle(float InDamage, float InSpeed, const FVector &InDirection, const FLinearColor &InColor);

protected:
	// Runs when the particle enters the level and connects its overlap event.
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

	bool bHasDealtDamage = false; // Flag to ensure damage is applied only once

	UFUNCTION()
	// Damages the player or destroys the particle when it hits a surface.
	void OnParticleOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);
};
