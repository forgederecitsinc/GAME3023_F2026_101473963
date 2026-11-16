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

 // Called every frame
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

 // InitializeParticle: initialize particle
	void InitializeParticle(float InDamage, float InSpeed, const FVector& InDirection, const FLinearColor& InColor);

protected:
 // Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMat;

	bool bHasDealtDamage = false;

	UFUNCTION()
 // Collision overlap callback
	void OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
