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
	PrimaryActorTick.bCanEverTick = true;

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

// SyncIDAndColor: keeps the bushing IDs synchronized and updates the default neon color.
void ATransformerBushing::SyncIDAndColor(int32 NewID)
{
	BushingID = NewID;
	TargetID = NewID;
	if (!bUseCustomNeonColor)
	{
		NeonLitColor = GetColorForID(BushingID);
	}
}

// OnConstruction: rebuilds the bushing appearance in the editor.
void ATransformerBushing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SyncIDAndColor(BushingID);
	UpdateAppearance();
}

#if WITH_EDITOR
// PostEditChangeProperty: updates the bushing after an editor setting changes.
void ATransformerBushing::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, TargetID))
	{
		SyncIDAndColor(TargetID);
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, BushingID) ||
			 PropName == GET_MEMBER_NAME_CHECKED(ATransformerBushing, bUseCustomNeonColor))
	{
		SyncIDAndColor(BushingID);
	}

	UpdateAppearance();
}
#endif

// BeginPlay: synchronizes the ID, finds matching targets, and updates appearance.
void ATransformerBushing::BeginPlay()
{
	Super::BeginPlay();

	SyncIDAndColor(BushingID);
	BindToMatchingTargets();
	UpdateAppearance();
}

// Tick: searches for newly spawned matching targets every two seconds.
void ATransformerBushing::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TargetCheckTimer += DeltaTime;
	if (TargetCheckTimer >= 2.0f)
	{
		TargetCheckTimer = 0.0f;
		BindToMatchingTargets();
	}
}

// BindTarget: connects this bushing to a matching laser target.
void ATransformerBushing::BindTarget(ALaserTarget* Target)
{
	if (!Target || Target->TargetID != BushingID)
	{
		return;
	}

	Target->OnLaserHitStateChanged.RemoveDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);
	Target->OnLaserHitStateChanged.AddDynamic(this, &ATransformerBushing::OnTargetHitStateChanged);

	if (Target->bIsHitByLaser)
	{
		SetLitUp(true);
	}
}

// BindToMatchingTargets: searches for matching targets in the level.
void ATransformerBushing::BindToMatchingTargets()
{
	if (LinkedLaserTarget)
	{
		BindTarget(LinkedLaserTarget);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundTargets;
	UGameplayStatics::GetAllActorsOfClass(World, ALaserTarget::StaticClass(), FoundTargets);
	for (AActor* Actor : FoundTargets)
	{
		BindTarget(Cast<ALaserTarget>(Actor));
	}
}

// OnTargetHitStateChanged: turns the bushing on or off when a matching target changes.
void ATransformerBushing::OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit)
{
	if (Target && Target->TargetID == BushingID)
	{
		SetLitUp(bIsHit);
	}
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

		LiveWireUtils::ShowScreenMessage(-1, 4.0f, GetActiveLitColor().ToFColor(true),
			FString::Printf(TEXT("[TRANSFORMER BUSHING %d] NEON ACTIVATED! (Matching Target Hit)"), BushingID));
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

		LiveWireUtils::ShowScreenMessage(-1, 2.5f, FColor::Silver,
			FString::Printf(TEXT("[TRANSFORMER BUSHING %d] NEON OFF (Laser disconnected)"), BushingID));
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

// GetColorForID: returns the LiveWire color mapped to the bushing ID.
FLinearColor ATransformerBushing::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

// GetActiveLitColor: returns a custom lit color or the ID-based color.
FLinearColor ATransformerBushing::GetActiveLitColor() const
{
	return bUseCustomNeonColor ? NeonLitColor : GetColorForID(BushingID);
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
