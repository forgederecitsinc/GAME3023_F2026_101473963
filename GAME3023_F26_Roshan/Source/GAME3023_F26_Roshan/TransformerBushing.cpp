// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: TransformerBushing.cpp
// Description: Implementation of the Transformer Bushing neon indicator.
// Listens for laser hit events on matching ALaserTarget and ignites with neon bloom.
// =============================================================================

#include "TransformerBushing.h"
#include "LaserTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATransformerBushing::ATransformerBushing()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 2. Static mesh component (mesh asset left unassigned so student assigns their custom mesh in BP_TransformerBushing)
	BushingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	BushingMesh->SetupAttachment(RootComponent);
	BushingMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Default unlit flat material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (FlatMatFinder.Succeeded())
	{
		UnlitMaterial = FlatMatFinder.Object;
		BaseMaterial = FlatMatFinder.Object;
	}

	// Glowing emissive neon material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> NeonMatFinder(TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	if (NeonMatFinder.Succeeded())
	{
		NeonMaterial = NeonMatFinder.Object;
	}
	else
	{
		NeonMaterial = UnlitMaterial;
	}

	// 3. Point light component for ambient neon illumination
	BushingLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BushingLight"));
	BushingLight->SetupAttachment(BushingMesh);
	BushingLight->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	BushingLight->SetIntensity(0.0f);
	BushingLight->SetAttenuationRadius(LightAttenuationRadius);
	BushingLight->SetSourceRadius(35.0f);
	BushingLight->SetSoftSourceRadius(50.0f);
	BushingLight->SetCastShadows(true);
	BushingLight->SetVisibility(false);
}

void ATransformerBushing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TargetID = BushingID;
	if (!bUseCustomNeonColor)
	{
		NeonLitColor = GetColorForID(BushingID);
	}

	UpdateAppearance();
}

#if WITH_EDITOR
void ATransformerBushing::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, BushingID))
	{
		TargetID = BushingID;
		if (!bUseCustomNeonColor)
		{
			NeonLitColor = GetColorForID(BushingID);
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, TargetID))
	{
		BushingID = TargetID;
		if (!bUseCustomNeonColor)
		{
			NeonLitColor = GetColorForID(BushingID);
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, bUseCustomNeonColor))
	{
		if (!bUseCustomNeonColor)
		{
			NeonLitColor = GetColorForID(BushingID);
		}
	}

	UpdateAppearance();
}
#endif

void ATransformerBushing::BeginPlay()
{
	Super::BeginPlay();

	TargetID = BushingID;
	if (!bUseCustomNeonColor)
	{
		NeonLitColor = GetColorForID(BushingID);
	}

	// Bind to matching laser target(s)
	BindToMatchingTargets();

	// Initialize visual appearance
	UpdateAppearance();
}

void ATransformerBushing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Safeguard for World Partition streaming: periodically rebind if not yet connected
	static float TargetCheckTimer = 0.0f;
	TargetCheckTimer += DeltaTime;
	if (TargetCheckTimer >= 2.0f)
	{
		TargetCheckTimer = 0.0f;
		BindToMatchingTargets();
	}
}

void ATransformerBushing::BindToMatchingTargets()
{
	if (LinkedLaserTarget)
	{
		LinkedLaserTarget->OnLaserHitStateChanged.RemoveDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);
		LinkedLaserTarget->OnLaserHitStateChanged.AddDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);

		if (LinkedLaserTarget->bIsHitByLaser && LinkedLaserTarget->TargetID == BushingID)
		{
			SetLitUp(true);
		}
		return;
	}

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundTargets;
		UGameplayStatics::GetAllActorsOfClass(World, ALaserTarget::StaticClass(), FoundTargets);
		for (AActor* Actor : FoundTargets)
		{
			if (ALaserTarget* Target = Cast<ALaserTarget>(Actor))
			{
				if (Target->TargetID == BushingID)
				{
					Target->OnLaserHitStateChanged.RemoveDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);
					Target->OnLaserHitStateChanged.AddDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);

					if (Target->bIsHitByLaser)
					{
						SetLitUp(true);
					}
				}
			}
		}
	}
}

void ATransformerBushing::OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit)
{
	if (!Target) return;

	// Only activate when the laser strikes the object matching this bushing's ID
	if (Target->TargetID == BushingID)
	{
		SetLitUp(bIsHit);
	}
}

void ATransformerBushing::SetLitUp(bool bLit)
{
	if (bStayLitOnceActivated && bIsLitUp && !bLit)
	{
		return; // Latched on permanently
	}

	if (bIsLitUp == bLit)
	{
		return;
	}

	bIsLitUp = bLit;

	if (bIsLitUp)
	{
		// Authentic neon ignition flicker
		if (bEnableNeonIgnitionFlicker && GetWorld() && GetWorld()->IsGameWorld())
		{
			StartNeonIgnitionFlicker();
		}
		else
		{
			UpdateAppearance();
		}

		if (LightUpSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, LightUpSound, GetActorLocation());
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.0f,
				GetActiveLitColor().ToFColor(true),
				FString::Printf(TEXT("[TRANSFORMER BUSHING %d] NEON ACTIVATED! (Matching Target Hit)"), BushingID)
			);
		}
	}
	else
	{
		// Cancel any running flicker timer
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(FlickerTimerHandle);
		}

		UpdateAppearance();

		if (PowerDownSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PowerDownSound, GetActorLocation());
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.5f,
				FColor::Silver,
				FString::Printf(TEXT("[TRANSFORMER BUSHING %d] NEON OFF (Laser disconnected)"), BushingID)
			);
		}
	}

	OnLitStateChanged(bIsLitUp);
	OnTransformerStateChanged.Broadcast(this, bIsLitUp);
}

void ATransformerBushing::StartNeonIgnitionFlicker()
{
	if (!GetWorld()) return;

	// Flash 1: immediate strike
	UpdateAppearance();

	FTimerManager& TM = GetWorld()->GetTimerManager();
	TM.ClearTimer(FlickerTimerHandle);

	// Brief 50ms dip
	TM.SetTimer(FlickerTimerHandle, [this]()
	{
		if (!bIsLitUp) return;

		// Brief dark flicker
		if (BushingLight) BushingLight->SetVisibility(false);
		if (BushingMesh)
		{
			for (int32 i = 0; i < BushingMesh->GetNumMaterials(); ++i)
			{
				if (UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(BushingMesh->GetMaterial(i)))
				{
					Dyn->SetVectorParameterValue(TEXT("Emissive Color"), FLinearColor::Black);
					Dyn->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
					Dyn->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
				}
			}
		}

		// Re-ignite to solid neon after another 60ms
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(FlickerTimerHandle, [this]()
			{
				if (bIsLitUp)
				{
					UpdateAppearance();
				}
			}, 0.06f, false);
		}
	}, 0.05f, false);
}

void ATransformerBushing::SetLaserHit(bool bHit, AActor* SourceActor)
{
	SetLitUp(bHit);
}

void ATransformerBushing::OnLitStateChanged_Implementation(bool bNewLit)
{
	// Default native implementation; can be overridden in Blueprints
}

FLinearColor ATransformerBushing::GetColorForID(int32 InID) const
{
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

FLinearColor ATransformerBushing::GetActiveLitColor() const
{
	return bUseCustomNeonColor ? NeonLitColor : GetColorForID(BushingID);
}

void ATransformerBushing::UpdateAppearance()
{
	const bool bShouldGlow = bIsLitUp || (bPreviewLitInEditor && GetWorld() && !GetWorld()->IsGameWorld());
	const FLinearColor LitColor = GetActiveLitColor();

	// 1. Update point light for ambient neon glow
	if (BushingLight)
	{
		BushingLight->SetLightColor(LitColor);
		BushingLight->SetAttenuationRadius(LightAttenuationRadius);
		BushingLight->SetIntensity(bShouldGlow ? LitLightIntensity : 0.0f);
		BushingLight->SetVisibility(bShouldGlow);
		BushingLight->SetSourceRadius(35.0f);
		BushingLight->SetSoftSourceRadius(50.0f);
	}

	// We do NOT color or override static meshes in C++!
	// Any materials assigned by the student in the Blueprint (BP_TransformerBushing) or on the mesh components
	// are preserved exactly as they are without being overwritten or tinted by C++.
}
