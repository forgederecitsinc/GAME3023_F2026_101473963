#include "TransformerBushing.h"
#include "LaserTarget.h"
#include "LiveWireUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Constructor: creates the bushing mesh and point light.
ATransformerBushing::ATransformerBushing()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BushingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	BushingMesh->SetupAttachment(RootComponent);
	BushingMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

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

// OnConstruction: rebuilds the bushing appearance in the editor.
void ATransformerBushing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateAppearance();
}

#if WITH_EDITOR
// PostEditChangeProperty: updates the bushing after an editor setting changes.
void ATransformerBushing::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateAppearance();
}
#endif

// BeginPlay: binds to any linked laser target and updates appearance.
void ATransformerBushing::BeginPlay()
{
	Super::BeginPlay();

	if (LinkedLaserTarget)
	{
		LinkedLaserTarget->OnLaserHitStateChanged.RemoveDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);
		LinkedLaserTarget->OnLaserHitStateChanged.AddDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);

		if (LinkedLaserTarget->bIsHitByLaser)
		{
			SetLitUp(true);
		}
	}

	UpdateAppearance();
}

// OnTargetHitStateChanged: turns the bushing on or off when the linked target changes.
void ATransformerBushing::OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit)
{
	SetLitUp(bIsHit);
}

// SetLitUp: changes the lit state, appearance, sound, and Blueprint events.
void ATransformerBushing::SetLitUp(bool bLit)
{
	if ((bStayLitOnceActivated && bIsLitUp && !bLit) || bIsLitUp == bLit)
	{
		return;
	}

	bIsLitUp = bLit;

	if (bIsLitUp)
	{
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
	}
	else
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FlickerTimerHandle);
		}

		UpdateAppearance();

		if (PowerDownSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PowerDownSound, GetActorLocation());
		}
	}

	OnLitStateChanged(bIsLitUp);
	OnTransformerStateChanged.Broadcast(this, bIsLitUp);
}

// SetMeshEmissive: applies an emissive color to the bushing material.
void ATransformerBushing::SetMeshEmissive(const FLinearColor& Color)
{
	if (!BushingMesh)
	{
		return;
	}

	for (int32 i = 0; i < BushingMesh->GetNumMaterials(); ++i)
	{
		if (UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(BushingMesh->GetMaterial(i)))
		{
			Dyn->SetVectorParameterValue(TEXT("Emissive Color"), Color);
			Dyn->SetVectorParameterValue(TEXT("EmissiveColor"), Color);
			Dyn->SetVectorParameterValue(TEXT("Emissive"), Color);
		}
	}
}

// StartNeonIgnitionFlicker: briefly switches the light off and back on.
void ATransformerBushing::StartNeonIgnitionFlicker()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UpdateAppearance();

	FTimerManager& TM = World->GetTimerManager();
	TM.ClearTimer(FlickerTimerHandle);

	TM.SetTimer(FlickerTimerHandle, [this]()
	{
		if (!bIsLitUp)
		{
			return;
		}

		if (BushingLight)
		{
			BushingLight->SetVisibility(false);
		}
		SetMeshEmissive(FLinearColor::Black);

		if (UWorld* InnerWorld = GetWorld())
		{
			InnerWorld->GetTimerManager().SetTimer(FlickerTimerHandle, [this]()
			{
				if (bIsLitUp)
				{
					UpdateAppearance();
				}
			}, 0.06f, false);
		}
	}, 0.05f, false);
}

// SetLaserHit: uses SetLitUp as the compatibility version of the laser hit function.
void ATransformerBushing::SetLaserHit(bool bHit, AActor* SourceActor)
{
	SetLitUp(bHit);
}

// OnLitStateChanged_Implementation: default Blueprint-native event implementation.
void ATransformerBushing::OnLitStateChanged_Implementation(bool bNewLit)
{
}

// GetActiveLitColor: returns the lit neon color.
FLinearColor ATransformerBushing::GetActiveLitColor() const
{
	return NeonLitColor;
}

// UpdateAppearance: updates the bushing light and glowing state.
void ATransformerBushing::UpdateAppearance()
{
	const bool bShouldGlow = bIsLitUp || (bPreviewLitInEditor && GetWorld() && !GetWorld()->IsGameWorld());
	const FLinearColor LitColor = GetActiveLitColor();

	if (BushingLight)
	{
		BushingLight->SetLightColor(LitColor);
		BushingLight->SetAttenuationRadius(LightAttenuationRadius);
		BushingLight->SetIntensity(bShouldGlow ? LitLightIntensity : 0.0f);
		BushingLight->SetVisibility(bShouldGlow);
		BushingLight->SetSourceRadius(35.0f);
		BushingLight->SetSoftSourceRadius(50.0f);
	}
}
