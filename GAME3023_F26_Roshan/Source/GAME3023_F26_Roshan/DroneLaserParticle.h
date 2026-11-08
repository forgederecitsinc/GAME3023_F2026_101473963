// roshan - 101473963 - game3023 game engines 3
// drone laser particle actor - dropped/fired by AEnemyDrone when the player is detected under its light cone
// travels downward toward the detected player and inflicts instance-editable damage on contact

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DroneLaserParticle.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UCLASS()
class GAME3023_F26_ROSHAN_API ADroneLaserParticle : public AActor
{
	GENERATED_BODY()
	
public:	
	ADroneLaserParticle();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	// Collision sphere for detecting player or ground impact
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	// Glowing laser/bomb particle mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ParticleMesh;

	// Point light for glowing laser bomb effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> ParticleLight;

	// Damage inflicted on the player when this particle hits (passed from the owning AEnemyDrone instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float DamageAmount = 10.0f;

	// Downward travel speed of the laser bomb particle (in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float FallSpeed = 1400.0f;

	// Direction of travel (defaults to straight down or aimed toward the player beneath the drone)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FVector MoveDirection = FVector(0.0f, 0.0f, -1.0f);

	// Visual color of the laser particle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FLinearColor ParticleColor = FLinearColor(1.0f, 0.08f, 0.08f);

	// Initializes the particle's damage, speed, direction, and glow color right after spawning
	void InitializeParticle(float InDamage, float InSpeed, const FVector& InDirection, const FLinearColor& InColor);

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

	bool bHasDealtDamage = false;

	UFUNCTION()
	void OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
