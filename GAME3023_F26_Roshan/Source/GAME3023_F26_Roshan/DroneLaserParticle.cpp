// roshan - 101473963 - game3023 game engines 3
// drone laser particle implementation - small glowing laser/bomb droplet dropped by AEnemyDrone

#include "DroneLaserParticle.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "CombatDamageable.h"

ADroneLaserParticle::ADroneLaserParticle()
{
	PrimaryActorTick.bCanEverTick = true;

	// Collision sphere root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(24.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	// Small glowing laser droplet / bomb mesh
	ParticleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParticleMesh"));
	ParticleMesh->SetupAttachment(CollisionSphere);
	ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ParticleMesh->SetCastShadow(false);
	// Elongated laser bolt / bomb droplet shape
	ParticleMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.45f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		ParticleMesh->SetStaticMesh(SphereFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LaserMatFinder(TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (LaserMatFinder.Succeeded())
	{
		ParticleMesh->SetMaterial(0, LaserMatFinder.Object);
	}
	else if (FlatMatFinder.Succeeded())
	{
		ParticleMesh->SetMaterial(0, FlatMatFinder.Object);
	}

	// Point light so each dropped laser particle casts a visible glow
	ParticleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ParticleLight"));
	ParticleLight->SetupAttachment(CollisionSphere);
	ParticleLight->Intensity = 4000.0f;
	ParticleLight->AttenuationRadius = 220.0f;
	ParticleLight->SetLightColor(ParticleColor);
	ParticleLight->SetCastShadows(false);

	// Auto-destroy after 3 seconds if it misses everything
	InitialLifeSpan = 3.0f;
}

void ADroneLaserParticle::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ADroneLaserParticle::OnParticleOverlap);
	}

	InitializeParticle(DamageAmount, FallSpeed, MoveDirection, ParticleColor);
}

void ADroneLaserParticle::InitializeParticle(float InDamage, float InSpeed, const FVector& InDirection, const FLinearColor& InColor)
{
	DamageAmount = InDamage;
	FallSpeed = InSpeed;
	MoveDirection = InDirection.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector(0.0f, 0.0f, -1.0f);
	}
	ParticleColor = InColor;

	// Orient the elongated laser particle along its travel vector
	SetActorRotation(MoveDirection.Rotation() + FRotator(90.0f, 0.0f, 0.0f));

	if (ParticleLight)
	{
		ParticleLight->SetLightColor(ParticleColor);
	}

	if (ParticleMesh && ParticleMesh->GetMaterial(0))
	{
		DynamicMat = ParticleMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMat)
		{
			DynamicMat->SetVectorParameterValue(TEXT("Color"), ParticleColor);
			DynamicMat->SetVectorParameterValue(TEXT("BaseColor"), ParticleColor);
			DynamicMat->SetVectorParameterValue(TEXT("EmissiveColor"), ParticleColor * 25.0f);
		}
	}
}

void ADroneLaserParticle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasDealtDamage)
	{
		return;
	}

	const FVector StartPos = GetActorLocation();
	const FVector EndPos = StartPos + (MoveDirection * FallSpeed * DeltaTime);

	FHitResult SweepHit;
	SetActorLocation(EndPos, true, &SweepHit);

	if (SweepHit.bBlockingHit && SweepHit.GetActor() && SweepHit.GetActor() != GetOwner())
	{
		OnParticleOverlap(CollisionSphere, SweepHit.GetActor(), SweepHit.GetComponent(), SweepHit.Item, true, SweepHit);
	}
}

void ADroneLaserParticle::OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasDealtDamage || !IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// Ignore other laser particles
	if (OtherActor->IsA(ADroneLaserParticle::StaticClass()))
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == PlayerPawn)
	{
		bHasDealtDamage = true;

		// Inflict configured instance-editable damage to the player's health
		UGameplayStatics::ApplyDamage(
			OtherActor,
			DamageAmount,
			GetInstigatorController(),
			GetOwner() ? GetOwner() : this,
			UDamageType::StaticClass()
		);

		Destroy();
		return;
	}

	// Destroy the particle when it hits the floor or solid static world geometry
	if (OtherComp && (OtherComp->GetCollisionObjectType() == ECC_WorldStatic || OtherComp->GetCollisionObjectType() == ECC_WorldDynamic))
	{
		bHasDealtDamage = true;
		Destroy();
	}
}
