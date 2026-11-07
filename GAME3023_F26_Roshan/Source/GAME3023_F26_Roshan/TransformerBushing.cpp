// roshan - 101473963 - game3023 game engines 3
// transformer bushing cpp - lights up like a neon light when the matching target is hit
// supports custom user-chosen neon colors and authentic neon tube bloom

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

	// 2. Static mesh component
	BushingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	BushingMesh->SetupAttachment(RootComponent);
	BushingMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// Default shape mesh asset (cylinder)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BushingMesh->SetStaticMesh(CylinderFinder.Object);
	}

	// Default unlit flat material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (FlatMatFinder.Succeeded())
	{
		UnlitMaterial = FlatMatFinder.Object;
		BaseMaterial = FlatMatFinder.Object;
		BushingMesh->SetMaterial(0, UnlitMaterial);
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
	const FLinearColor CurrentColor = bShouldGlow ? LitColor : NeonUnlitColor;
	const float CurrentEmissiveMult = bShouldGlow ? LitEmissiveMultiplier : UnlitEmissiveMultiplier;
	const FLinearColor EmissiveCol = LitColor * CurrentEmissiveMult;

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

	// 2. Choose target material template
	UMaterialInterface* TargetMatTemplate = bShouldGlow ? (NeonMaterial ? NeonMaterial.Get() : UnlitMaterial.Get())
	                                                   : (UnlitMaterial ? UnlitMaterial.Get() : NeonMaterial.Get());

	auto ApplyNeonToMesh = [this, TargetMatTemplate, &CurrentColor, &EmissiveCol, CurrentEmissiveMult, bShouldGlow](UStaticMeshComponent* MeshComp)
	{
		if (!MeshComp) return;

		const int32 NumMats = FMath::Max(1, MeshComp->GetNumMaterials());
		for (int32 Slot = 0; Slot < NumMats; ++Slot)
		{
			if (bSwapMaterialsForNeon && TargetMatTemplate)
			{
				UMaterialInterface* ExistingMat = MeshComp->GetMaterial(Slot);
				if (!ExistingMat)
				{
					MeshComp->SetMaterial(Slot, TargetMatTemplate);
				}
				else if (UMaterialInstanceDynamic* ExistDyn = Cast<UMaterialInstanceDynamic>(ExistingMat))
				{
					if (ExistDyn->Parent != TargetMatTemplate)
					{
						MeshComp->SetMaterial(Slot, TargetMatTemplate);
					}
				}
				else if (ExistingMat != TargetMatTemplate)
				{
					MeshComp->SetMaterial(Slot, TargetMatTemplate);
				}
			}

			UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(Slot));
			if (!Dyn)
			{
				Dyn = MeshComp->CreateAndSetMaterialInstanceDynamic(Slot);
			}

			if (Dyn)
			{
				Dyn->SetVectorParameterValue(TEXT("Base Color"), CurrentColor);
				Dyn->SetVectorParameterValue(TEXT("BaseColor"), CurrentColor);
				Dyn->SetVectorParameterValue(TEXT("Color"), CurrentColor);
				Dyn->SetVectorParameterValue(TEXT("Tint"), CurrentColor);

				Dyn->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveCol);
				Dyn->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveCol);
				Dyn->SetVectorParameterValue(TEXT("Emissive"), EmissiveCol);
				Dyn->SetScalarParameterValue(TEXT("EmissiveIntensity"), CurrentEmissiveMult);
			}
		}
	};

	// Apply to primary BushingMesh
	ApplyNeonToMesh(BushingMesh);

	// Apply to any other StaticMeshComponents attached (e.g. user-added blueprint parts!)
	TInlineComponentArray<UStaticMeshComponent*> AllMeshComponents(this);
	for (UStaticMeshComponent* ExtraMesh : AllMeshComponents)
	{
		if (ExtraMesh && ExtraMesh != BushingMesh)
		{
			ApplyNeonToMesh(ExtraMesh);
		}
	}
}
