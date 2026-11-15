// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3

#include "DroneLaserParticle.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

ADroneLaserParticle::ADroneLaserParticle()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(24.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	ParticleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParticleMesh"));
	ParticleMesh->SetupAttachment(CollisionSphere);
	ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ParticleMesh->SetCastShadow(false);
	ParticleMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.45f));

	ParticleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ParticleLight"));
	ParticleLight->SetupAttachment(CollisionSphere);
	ParticleLight->Intensity = 4000.0f;
	ParticleLight->AttenuationRadius = 220.0f;
	ParticleLight->SetLightColor(ParticleColor);
	ParticleLight->SetCastShadows(false);

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
	SetActorRotation(MoveDirection.Rotation() + FRotator(90.0f, 0.0f, 0.0f));

	if (ParticleLight)
	{
		ParticleLight->SetLightColor(ParticleColor);
	}
}

void ADroneLaserParticle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasDealtDamage)
	{
		return;
	}

	const FVector EndPos = GetActorLocation() + (MoveDirection * FallSpeed * DeltaTime);
	FHitResult SweepHit;
	SetActorLocation(EndPos, true, &SweepHit);

	if (SweepHit.bBlockingHit && SweepHit.GetActor() && SweepHit.GetActor() != GetOwner())
	{
		OnParticleOverlap(CollisionSphere, SweepHit.GetActor(), SweepHit.GetComponent(), SweepHit.Item, true, SweepHit);
	}
}

void ADroneLaserParticle::OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bHasDealtDamage || !IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner() || OtherActor->IsA<ADroneLaserParticle>())
	{
		return;
	}

	if (OtherActor == UGameplayStatics::GetPlayerPawn(this, 0))
	{
		bHasDealtDamage = true;
		UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, GetInstigatorController(), GetOwner() ? GetOwner() : this, UDamageType::StaticClass());
		Destroy();
		return;
	}

	if (OtherComp && (OtherComp->GetCollisionObjectType() == ECC_WorldStatic || OtherComp->GetCollisionObjectType() == ECC_WorldDynamic))
	{
		bHasDealtDamage = true;
		Destroy();
	}
}
