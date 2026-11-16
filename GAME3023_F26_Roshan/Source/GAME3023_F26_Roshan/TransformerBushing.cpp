#include "TransformerBushing.h"
#include "LaserTarget.h"
#include "LiveWireUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Sets up bushing mesh, spark points, and interaction zone
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

// SyncIDAndColor: updates dynamic material colors based on ID
void ATransformerBushing::SyncIDAndColor(int32 NewID)
{
	BushingID = NewID;
	TargetID = NewID;
	if (!bUseCustomNeonColor)
	{
		NeonLitColor = GetColorForID(BushingID);
	}
}

// Updates editor visual appearance and ID colors
void ATransformerBushing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SyncIDAndColor(BushingID);
	UpdateAppearance();
}

#if WITH_EDITOR
// Refreshes bushing settings on editor property change
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

// Initializes ID, dynamic materials, and spark particles
void ATransformerBushing::BeginPlay()
{
	Super::BeginPlay();

	SyncIDAndColor(BushingID);
	BindToMatchingTargets();
	UpdateAppearance();
}

// Updates electrical sparks and laser connection checks
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

// BindTarget: handles bind target
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

// BindToMatchingTargets: handles bind to matching targets
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

// OnTargetHitStateChanged: handles on target hit state changed
void ATransformerBushing::OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit)
{
	if (Target && Target->TargetID == BushingID)
	{
		SetLitUp(bIsHit);
	}
}

// SetLitUp: handles set lit up
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

// SetMeshEmissive: handles set mesh emissive
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

// StartNeonIgnitionFlicker: handles start neon ignition flicker
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

// SetLaserHit: controls laser trace and hit detection
void ATransformerBushing::SetLaserHit(bool bHit, AActor* SourceActor)
{
	SetLitUp(bHit);
}

// OnLitStateChanged_Implementation: handles on lit state changed implementation
void ATransformerBushing::OnLitStateChanged_Implementation(bool bNewLit)
{
}

// Returns mapped linear color for bushing 4-digit ID
FLinearColor ATransformerBushing::GetColorForID(int32 InID) const
{
	return LiveWireUtils::GetColorForID(InID);
}

// GetActiveLitColor: updates dynamic material colors based on ID
FLinearColor ATransformerBushing::GetActiveLitColor() const
{
	return bUseCustomNeonColor ? NeonLitColor : GetColorForID(BushingID);
}

// UpdateAppearance: handles update appearance
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
