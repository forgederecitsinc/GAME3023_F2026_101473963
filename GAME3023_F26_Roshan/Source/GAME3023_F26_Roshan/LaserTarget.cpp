// roshan - 101473963 - game3023 game engines 3
// laser target cpp - industrial sharp-edged laser receiver matching the substation arena blockout aesthetic
// designed to receive and detect incoming laser beams from RedPowerCylinder laser turrets

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

	// 1. Root scene component (origin at Z=0, sits at the top face of the base plinth)
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Basic shape mesh assets
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object.Get() : nullptr;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object.Get() : CubeMesh;

	// Prototype flat material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	UMaterialInterface* BaseFlatMat = FlatMatFinder.Succeeded() ? FlatMatFinder.Object.Get() : nullptr;

	// Default mesh assets exposed for editing in BP_LaserTarget
	BasePedestalMeshAsset = CubeMesh;
	PostColumnMeshAsset = CylinderMesh;
	NeckCollarMeshAsset = CubeMesh;
	TargetBodyMeshAsset = CubeMesh;
	TopCapMeshAsset = CubeMesh;
	SensorMeshAsset = CubeMesh;
	BaseMaterial = BaseFlatMat;

	// 1. Base pedestal slab - clean light industrial concrete foundation on floor
	BasePedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePedestalMesh"));
	BasePedestalMesh->SetupAttachment(RootComponent);
	BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	BasePedestalMesh->SetRelativeScale3D(BasePedestalScale);
	BasePedestalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BasePedestalMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) BasePedestalMesh->SetMaterial(0, BaseFlatMat);

	// 2. Upright pillar column - clean silver alloy cylinder
	PostColumnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostColumnMesh"));
	PostColumnMesh->SetupAttachment(RootComponent);
	PostColumnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	PostColumnMesh->SetRelativeScale3D(PillarScale);
	PostColumnMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CylinderMesh) PostColumnMesh->SetStaticMesh(CylinderMesh);
	if (BaseFlatMat) PostColumnMesh->SetMaterial(0, BaseFlatMat);

	// 3. Neck collar plate - dark graphite collar platform beneath the receiver head
	NeckCollarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NeckCollarMesh"));
	NeckCollarMesh->SetupAttachment(RootComponent);
	NeckCollarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
	NeckCollarMesh->SetRelativeScale3D(NeckCollarScale);
	NeckCollarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) NeckCollarMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) NeckCollarMesh->SetMaterial(0, BaseFlatMat);

	// 4. Main target receiver body box - sharp rectangular block tinted with target color
	TargetBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetBodyMesh"));
	TargetBodyMesh->SetupAttachment(RootComponent);
	TargetBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 185.0f));
	TargetBodyMesh->SetRelativeScale3D(TargetBodyScale);
	TargetBodyMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) TargetBodyMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) TargetBodyMesh->SetMaterial(0, BaseFlatMat);

	// 5. Top cap box - dark square cap plate centered on top deck
	TopCapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopCapMesh"));
	TopCapMesh->SetupAttachment(RootComponent);
	TopCapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 247.5f));
	TopCapMesh->SetRelativeScale3D(TopCapScale);
	TopCapMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) TopCapMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) TopCapMesh->SetMaterial(0, BaseFlatMat);

	// 6. Side sensor / receiver box - dark rectangular sensor centered on the side face
	SensorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SensorMesh"));
	SensorMesh->SetupAttachment(RootComponent);
	SensorMesh->SetRelativeLocation(SensorBoxLocation);
	SensorMesh->SetRelativeScale3D(SensorBoxScale);
	SensorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) SensorMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) SensorMesh->SetMaterial(0, BaseFlatMat);
}

void ALaserTarget::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 1. Base pedestal
	if (BasePedestalMesh)
	{
		BasePedestalMesh->SetVisibility(bShowBaseStructure);
		BasePedestalMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -BasePedestalScale.Z * 50.0f));
		BasePedestalMesh->SetRelativeScale3D(BasePedestalScale);
		if (BasePedestalMeshAsset)
		{
			BasePedestalMesh->SetStaticMesh(BasePedestalMeshAsset);
		}
	}

	// 2. Upright pillar column
	if (PostColumnMesh)
	{
		PostColumnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, PillarScale.Z * 50.0f));
		PostColumnMesh->SetRelativeScale3D(PillarScale);
		if (PostColumnMeshAsset)
		{
			PostColumnMesh->SetStaticMesh(PostColumnMeshAsset);
		}
	}

	const float PillarTop = PillarScale.Z * 100.0f;
	const float CollarThickness = NeckCollarScale.Z * 100.0f;
	const float CollarCenter = PillarTop + (CollarThickness * 0.5f);
	const float CollarTop = PillarTop + CollarThickness;

	// 3. Neck collar
	if (NeckCollarMesh)
	{
		NeckCollarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, CollarCenter));
		NeckCollarMesh->SetRelativeScale3D(NeckCollarScale);
		if (NeckCollarMeshAsset)
		{
			NeckCollarMesh->SetStaticMesh(NeckCollarMeshAsset);
		}
	}

	const float BodyHeight = TargetBodyScale.Z * 100.0f;
	const float BodyCenter = CollarTop + (BodyHeight * 0.5f);
	const float BodyTop = CollarTop + BodyHeight;

	// 4. Target body
	if (TargetBodyMesh)
	{
		TargetBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BodyCenter));
		TargetBodyMesh->SetRelativeScale3D(TargetBodyScale);
		if (TargetBodyMeshAsset)
		{
			TargetBodyMesh->SetStaticMesh(TargetBodyMeshAsset);
		}
	}

	const float CapHeight = TopCapScale.Z * 100.0f;
	const float CapCenter = BodyTop + (CapHeight * 0.5f);

	// 5. Top cap
	if (TopCapMesh)
	{
		TopCapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, CapCenter));
		TopCapMesh->SetRelativeScale3D(TopCapScale);
		if (TopCapMeshAsset)
		{
			TopCapMesh->SetStaticMesh(TopCapMeshAsset);
		}
	}

	// 6. Side sensor box
	if (SensorMesh)
	{
		if (bAutoCenterSensor)
		{
			const float TargetBodyHalfWidthY = (TargetBodyScale.Y * 100.0f) * 0.5f;
			const float SensorHalfWidthY = (SensorBoxScale.Y * 100.0f) * 0.5f;
			SensorBoxLocation = FVector(0.0f, TargetBodyHalfWidthY + SensorHalfWidthY - 2.0f, BodyCenter);
		}

		SensorMesh->SetRelativeLocation(SensorBoxLocation);
		SensorMesh->SetRelativeScale3D(SensorBoxScale);
		if (SensorMeshAsset)
		{
			SensorMesh->SetStaticMesh(SensorMeshAsset);
		}
	}

	// Apply custom materials if assigned
	if (BasePedestalMaterial && BasePedestalMesh) BasePedestalMesh->SetMaterial(0, BasePedestalMaterial);
	if (PostColumnMaterial && PostColumnMesh) PostColumnMesh->SetMaterial(0, PostColumnMaterial);
	if (NeckCollarMaterial && NeckCollarMesh) NeckCollarMesh->SetMaterial(0, NeckCollarMaterial);
	if (TargetBodyMaterial && TargetBodyMesh) TargetBodyMesh->SetMaterial(0, TargetBodyMaterial);
	if (TopCapMaterial && TopCapMesh) TopCapMesh->SetMaterial(0, TopCapMaterial);
	if (SensorMaterial && SensorMesh) SensorMesh->SetMaterial(0, SensorMaterial);

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
	// NEVER create transient dynamic material instances on templates or CDOs in Blueprint Editor!
	// Creating transient MIDs during Blueprint compilation corrupts component template serialization and reverts materials.
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	const FLinearColor BodyColor = GetColorForID(TargetID);
	const FLinearColor SensorColor = FLinearColor(0.10f, 0.10f, 0.12f);
	const FLinearColor TopCapColor = FLinearColor(0.12f, 0.12f, 0.14f);
	const FLinearColor NeckCollarColor = FLinearColor(0.14f, 0.14f, 0.16f);
	const FLinearColor PillarColor = FLinearColor(0.80f, 0.82f, 0.86f);
	const FLinearColor PedestalColor = FLinearColor(0.82f, 0.82f, 0.85f);

	UMaterialInterface* BaseFlatMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));

	// Fallback to BaseMaterial only if component has no material at all
	if (BaseMaterial)
	{
		if (BasePedestalMesh && !BasePedestalMesh->GetMaterial(0)) BasePedestalMesh->SetMaterial(0, BaseMaterial);
		if (PostColumnMesh && !PostColumnMesh->GetMaterial(0)) PostColumnMesh->SetMaterial(0, BaseMaterial);
		if (NeckCollarMesh && !NeckCollarMesh->GetMaterial(0)) NeckCollarMesh->SetMaterial(0, BaseMaterial);
		if (TargetBodyMesh && !TargetBodyMesh->GetMaterial(0)) TargetBodyMesh->SetMaterial(0, BaseMaterial);
		if (TopCapMesh && !TopCapMesh->GetMaterial(0)) TopCapMesh->SetMaterial(0, BaseMaterial);
		if (SensorMesh && !SensorMesh->GetMaterial(0)) SensorMesh->SetMaterial(0, BaseMaterial);
	}

	auto SetMeshColor = [BaseFlatMat](UStaticMeshComponent* MeshComp, const FLinearColor& BaseCol, const FLinearColor& EmissiveCol, bool bForceTint = false) -> UMaterialInstanceDynamic*
	{
		if (!MeshComp) return nullptr;
		UMaterialInterface* CurrentMat = MeshComp->GetMaterial(0);
		if (!CurrentMat) return nullptr;

		const bool bIsDefaultFlatMat = (CurrentMat == BaseFlatMat) || CurrentMat->GetName().Contains(TEXT("FlatCol"));

		const bool bHasEmissive = (EmissiveCol.R > 0.001f || EmissiveCol.G > 0.001f || EmissiveCol.B > 0.001f);
		if (!bIsDefaultFlatMat && !bForceTint && !bHasEmissive)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(CurrentMat);
		if (!Dyn)
		{
			Dyn = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (Dyn)
		{
			if (bIsDefaultFlatMat || bForceTint)
			{
				Dyn->SetVectorParameterValue(TEXT("Base Color"), BaseCol);
				Dyn->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
				Dyn->SetVectorParameterValue(TEXT("Color"), BaseCol);
				Dyn->SetVectorParameterValue(TEXT("Tint"), BaseCol);
			}

			if (bHasEmissive)
			{
				Dyn->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveCol);
				Dyn->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveCol);
				Dyn->SetVectorParameterValue(TEXT("Emissive"), EmissiveCol);
			}
		}
		return Dyn;
	};

	// 1. Base pedestal slab (clean industrial concrete/cream)
	PedestalDynamicMaterial = SetMeshColor(BasePedestalMesh, PedestalColor, FLinearColor::Black);

	// 2. Upright pillar column (silver metallic alloy)
	PillarDynamicMaterial = SetMeshColor(PostColumnMesh, PillarColor, FLinearColor::Black);

	// 3. Neck collar plate (dark industrial charcoal)
	CollarDynamicMaterial = SetMeshColor(NeckCollarMesh, NeckCollarColor, FLinearColor::Black);

	// 4. Main target receiver body box (vibrant target color, glows on hit)
	const FLinearColor BodyEmissive = (bIsHitByLaser && bGlowWhenHit) ? (BodyColor * 3.0f) : FLinearColor::Black;
	BodyDynamicMaterial = SetMeshColor(TargetBodyMesh, BodyColor, BodyEmissive, true);

	// 5. Top cap box (dark charcoal)
	CapDynamicMaterial = SetMeshColor(TopCapMesh, TopCapColor, FLinearColor::Black);

	// 6. Side sensor / receiver box (dark charcoal, glows on hit)
	const FLinearColor SensorEmissive = (bIsHitByLaser && bGlowWhenHit) ? (BodyColor * 5.0f) : FLinearColor::Black;
	SensorDynamicMaterial = SetMeshColor(SensorMesh, SensorColor, SensorEmissive, true);
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
