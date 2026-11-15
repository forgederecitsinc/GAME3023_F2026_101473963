// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: DroneLaserParticle.h
// Description: Drone Laser Bomb / Particle projectile actor.
// Dropped / fired by AEnemyDrone when the player is detected under its red spotlight.
// =============================================================================

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
	// Sets default values for this actor's properties
	ADroneLaserParticle();

protected:
	// Called when the particle is spawned into the world
	virtual void BeginPlay() override;

public:	
	// Called every frame to move the particle through the air
	virtual void Tick(float DeltaTime) override;

	// =========================================================================
	// 📦 COMPONENTS (The physical parts of the projectile)
	// =========================================================================

	// Collision sphere for detecting when the laser hits the player or floor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	// Glowing laser/bomb particle mesh (elongated glowing droplet shape)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> ParticleMesh;

	// Point light that makes the dropped laser droplet cast a bright glow in the environment
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> ParticleLight;

	// =========================================================================
	// 🎮 TWEAKABLE DESIGNER SETTINGS (Change these in Blueprint or here in code!)
	// =========================================================================

	// ✏️ CHANGE THIS: How much damage this laser bomb deals when it hits the player!
	// Default is 10.0 damage points. (Player starts with 100 max health).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float DamageAmount = 10.0f;

	// ✏️ CHANGE THIS: How fast the projectile flies downward towards the player (in cm/s).
	// Default is 1400.0 cm/s. Increase for a faster, harder-to-dodge drop!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	float FallSpeed = 1400.0f;

	// Direction the projectile travels (aimed straight down or toward player position)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FVector MoveDirection = FVector(0.0f, 0.0f, -1.0f);

	// ✏️ CHANGE THIS: Glow color of the laser bomb projectile (matches drone laser light)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Particle", meta = (ExposeOnSpawn = "true"))
	FLinearColor ParticleColor = FLinearColor(1.0f, 0.08f, 0.08f);

	// =========================================================================
	// 🛠️ INITIALIZATION & LOGIC
	// =========================================================================

	// Called right after spawning by AEnemyDrone to pass in customized damage, speed, aim direction, and color
	void InitializeParticle(float InDamage, float InSpeed, const FVector& InDirection, const FLinearColor& InColor);

private:
	// Dynamic material instance used to tint the glowing emissive material at runtime
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

	// Guard flag to make sure this projectile only deals damage once before being destroyed
	bool bHasDealtDamage = false;

	// Overlap callback triggered when the projectile touches an actor or wall
	UFUNCTION()
	void OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
