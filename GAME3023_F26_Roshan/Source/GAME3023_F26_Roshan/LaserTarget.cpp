// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: LaserTarget.cpp
// Description: Implementation of the Laser Receiver Target.
// Detects laser raycast impacts from RedPowerCylinder and broadcasts hit state.
// =============================================================================

#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ALaserTarget::ALaserTarget()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void ALaserTarget::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyColors();
}

void ALaserTarget::BeginPlay()
{
	Super::BeginPlay();

	ApplyColors();
}

void ALaserTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

FLinearColor ALaserTarget::GetColorForID(int32 InID) const
{
	// vibrant saturated color palette matching RedPowerCylinder
	switch (InID)
	{
		case 1001: return FLinearColor(1.00f, 0.10f, 0.12f); // vibrant cherry red
		case 1002: return FLinearColor(0.08f, 0.95f, 0.18f); // vibrant electric emerald green
		case 1003: return FLinearColor(0.08f, 0.45f, 1.00f); // vibrant cobalt electric blue
		case 1004: return FLinearColor(1.00f, 0.82f, 0.02f); // vibrant hazard caution yellow
		case 1005: return FLinearColor(1.00f, 0.42f, 0.02f); // vibrant safety neon orange
		case 1006: return FLinearColor(0.78f, 0.10f, 1.00f); // vibrant neon electric violet
		case 1007: return FLinearColor(0.02f, 0.92f, 0.98f); // vibrant bright laser cyan
		case 1008: return FLinearColor(1.00f, 0.10f, 0.65f); // vibrant hot neon magenta
		default:
		{
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 240, 255);
		}
	}
}

void ALaserTarget::ApplyColors()
{
	// We do NOT color or override static meshes in C++!
	// Any materials assigned by the student in the Blueprint (BP_LaserTarget) or on the mesh components
	// are preserved exactly as they are without being overwritten or tinted by C++.
}

void ALaserTarget::SetLaserHit(bool bHit, AActor* SourceActor)
{
	if (bIsHitByLaser == bHit)
	{
		return;
	}

	bIsHitByLaser = bHit;

	// Update material glow
	ApplyColors();

	// Broadcast delegate event
	OnLaserHitStateChanged.Broadcast(this, bIsHitByLaser);

	// Also directly notify any matching TransformerBushings in the world
	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundBushings;
		UGameplayStatics::GetAllActorsOfClass(World, ATransformerBushing::StaticClass(), FoundBushings);
		for (AActor* BushingActor : FoundBushings)
		{
			if (ATransformerBushing* Bushing = Cast<ATransformerBushing>(BushingActor))
			{
				if (Bushing->BushingID == TargetID)
				{
					Bushing->SetLitUp(bIsHitByLaser);
				}
			}
		}
	}
}
