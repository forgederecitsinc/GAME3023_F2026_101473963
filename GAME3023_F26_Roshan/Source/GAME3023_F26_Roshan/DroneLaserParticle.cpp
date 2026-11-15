// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: DroneLaserParticle.cpp
// Description: Implementation of the Drone Laser Bomb projectile.
// Moves downward every frame, checks for overlap with player or geometry,
// deals damage on contact, and self-destructs.
// =============================================================================

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

// -----------------------------------------------------------------------------
// CONSTRUCTOR: Sets up collision, visuals, point light, and lifespan
// -----------------------------------------------------------------------------
ADroneLaserParticle::ADroneLaserParticle()
{
	// Enable Tick() so this particle can update its position every frame
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root Collision Sphere: detects overlapping actors (player, obstacles)
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(24.0f); // 24cm hit radius
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	// 2. Visible 3D Mesh: shaped like an elongated laser teardrop / bolt
	ParticleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ParticleMesh"));
	ParticleMesh->SetupAttachment(CollisionSphere);
	ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Collision is handled by the sphere!
	ParticleMesh->SetCastShadow(false);
	// Scale X and Y thin, Z stretched out to look like a falling laser bolt
	ParticleMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.45f));

	// Note: Static mesh and material left unassigned in C++ so the student can assign their own
	// custom projectile mesh / effect inside BP_DroneLaserParticle!

	// 3. Dynamic Point Light: illuminates the floor/walls as the bomb drops
	ParticleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ParticleLight"));
	ParticleLight->SetupAttachment(CollisionSphere);
	ParticleLight->Intensity = 4000.0f;           // Glow brightness
	ParticleLight->AttenuationRadius = 220.0f;    // Glow distance
	ParticleLight->SetLightColor(ParticleColor);
	ParticleLight->SetCastShadows(false);

	// 4. Auto-Destroy Lifespan: if it doesn't hit anything in 3 seconds, destroy it so it doesn't linger forever
	// ✏️ CHANGE THIS: Increase if the drone drops from higher heights and needs more time to reach the floor!
	InitialLifeSpan = 3.0f;
}

// -----------------------------------------------------------------------------
// BEGIN PLAY: Binds overlap delegate and applies initial settings
// -----------------------------------------------------------------------------
void ADroneLaserParticle::BeginPlay()
{
	Super::BeginPlay();

	// Listen for overlap events on the collision sphere
	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ADroneLaserParticle::OnParticleOverlap);
	}

	// Initialize with default or spawn-passed values
	InitializeParticle(DamageAmount, FallSpeed, MoveDirection, ParticleColor);
}

// -----------------------------------------------------------------------------
// INITIALIZE: Called right after spawn to configure damage, speed, aim & color
// -----------------------------------------------------------------------------
void ADroneLaserParticle::InitializeParticle(float InDamage, float InSpeed, const FVector& InDirection, const FLinearColor& InColor)
{
	DamageAmount = InDamage;
	FallSpeed = InSpeed;
	MoveDirection = InDirection.GetSafeNormal();

	// If no direction provided, default to dropping straight down along -Z
	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector(0.0f, 0.0f, -1.0f);
	}
	ParticleColor = InColor;

	// Orient the elongated mesh to point forward in its travel direction
	SetActorRotation(MoveDirection.Rotation() + FRotator(90.0f, 0.0f, 0.0f));

	// Update light color to match the laser beam
	if (ParticleLight)
	{
		ParticleLight->SetLightColor(ParticleColor);
	}

	// Note: We do NOT color or override static mesh materials in C++.
	// Materials assigned in BP_DroneLaserParticle are preserved.
}

// -----------------------------------------------------------------------------
// TICK: Moves the projectile through space each frame with sweep collision
// -----------------------------------------------------------------------------
void ADroneLaserParticle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If we already hit something and dealt damage, don't move anymore
	if (bHasDealtDamage)
	{
		return;
	}

	// Calculate this frame's travel offset: DeltaPosition = Velocity * DeltaTime
	const FVector StartPos = GetActorLocation();
	const FVector EndPos = StartPos + (MoveDirection * FallSpeed * DeltaTime);

	// Sweep move: prevents the projectile from tunneling through thin geometry
	FHitResult SweepHit;
	SetActorLocation(EndPos, true, &SweepHit);

	// If sweeping struck a blocking surface, trigger the overlap handler immediately
	if (SweepHit.bBlockingHit && SweepHit.GetActor() && SweepHit.GetActor() != GetOwner())
	{
		OnParticleOverlap(CollisionSphere, SweepHit.GetActor(), SweepHit.GetComponent(), SweepHit.Item, true, SweepHit);
	}
}

// -----------------------------------------------------------------------------
// OVERLAP HANDLER: Checks what was hit, applies damage, and cleans up
// -----------------------------------------------------------------------------
void ADroneLaserParticle::OnParticleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Guard checks: don't double-hit, ignore invalid targets, and ignore the drone owner that fired it
	if (bHasDealtDamage || !IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// Ignore colliding with another laser particle
	if (OtherActor->IsA(ADroneLaserParticle::StaticClass()))
	{
		return;
	}

	// Check if we hit the player!
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (OtherActor == PlayerPawn)
	{
		bHasDealtDamage = true;

		// 💥 Apply damage to the player using Unreal's standard damage system
		// (This fires TakeDamage in AGAME3023_F26_RoshanCharacter)
		UGameplayStatics::ApplyDamage(
			OtherActor,
			DamageAmount,
			GetInstigatorController(),
			GetOwner() ? GetOwner() : this,
			UDamageType::StaticClass()
		);

		// Destroy the projectile immediately on player impact
		Destroy();
		return;
	}

	// If we hit the floor, catwalk, or static world environment, pop / destroy
	if (OtherComp && (OtherComp->GetCollisionObjectType() == ECC_WorldStatic || OtherComp->GetCollisionObjectType() == ECC_WorldDynamic))
	{
		bHasDealtDamage = true;
		Destroy();
	}
}
