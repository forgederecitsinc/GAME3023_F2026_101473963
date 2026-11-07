// roshan - 101473963 - game3023 game engines 3
// laser target cpp - industrial sharp-edged laser receiver matching the substation arena blockout aesthetic
// designed to receive and detect incoming laser beams from RedPowerCylinder laser turrets

#include "LaserTarget.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/KismetSystemLibrary.h"

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

	// 6. Side sensor / receiver box - dark rectangular sensor attached to side face flush with front
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
	}

	// 2. Upright pillar column
	if (PostColumnMesh)
	{
		PostColumnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, PillarScale.Z * 50.0f));
		PostColumnMesh->SetRelativeScale3D(PillarScale);
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
	}

	const float BodyHeight = TargetBodyScale.Z * 100.0f;
	const float BodyCenter = CollarTop + (BodyHeight * 0.5f);
	const float BodyTop = CollarTop + BodyHeight;

	// 4. Target body
	if (TargetBodyMesh)
	{
		TargetBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, BodyCenter));
		TargetBodyMesh->SetRelativeScale3D(TargetBodyScale);
	}

	const float CapHeight = TopCapScale.Z * 100.0f;
	const float CapCenter = BodyTop + (CapHeight * 0.5f);

	// 5. Top cap
	if (TopCapMesh)
	{
		TopCapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, CapCenter));
		TopCapMesh->SetRelativeScale3D(TopCapScale);
	}

	// 6. Side sensor box
	if (SensorMesh)
	{
		SensorMesh->SetRelativeLocation(SensorBoxLocation);
		SensorMesh->SetRelativeScale3D(SensorBoxScale);
	}

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
	const FLinearColor ChosenColor = bUseCustomColor ? CustomColor : GetColorForID(TargetID);

	// 1. Base pedestal slab (clean industrial concrete/cream)
	if (BasePedestalMesh)
	{
		PedestalDynamicMaterial = BasePedestalMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (PedestalDynamicMaterial)
		{
			PedestalDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PedestalColor);
			PedestalDynamicMaterial->SetVectorParameterValue(TEXT("Color"), PedestalColor);
		}
	}

	// 2. Pillar post column (silver metallic alloy)
	if (PostColumnMesh)
	{
		PillarDynamicMaterial = PostColumnMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (PillarDynamicMaterial)
		{
			PillarDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), PillarColor);
			PillarDynamicMaterial->SetVectorParameterValue(TEXT("Color"), PillarColor);
		}
	}

	// 3. Neck collar plate (dark industrial charcoal)
	if (NeckCollarMesh)
	{
		CollarDynamicMaterial = NeckCollarMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (CollarDynamicMaterial)
		{
			CollarDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), NeckCollarColor);
			CollarDynamicMaterial->SetVectorParameterValue(TEXT("Color"), NeckCollarColor);
		}
	}

	// 4. Main target receiver body box (vibrant target red)
	if (TargetBodyMesh)
	{
		BodyDynamicMaterial = TargetBodyMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (BodyDynamicMaterial)
		{
			BodyDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ChosenColor);
			BodyDynamicMaterial->SetVectorParameterValue(TEXT("Color"), ChosenColor);

			if (bIsHitByLaser && bGlowWhenHit)
			{
				BodyDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), ChosenColor * 3.0f);
				BodyDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), ChosenColor * 3.0f);
			}
			else
			{
				BodyDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
				BodyDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
			}
		}
	}

	// 5. Top cap box (dark charcoal)
	if (TopCapMesh)
	{
		CapDynamicMaterial = TopCapMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (CapDynamicMaterial)
		{
			CapDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), TopCapColor);
			CapDynamicMaterial->SetVectorParameterValue(TEXT("Color"), TopCapColor);
		}
	}

	// 6. Side sensor / receiver box (dark charcoal, glows on hit)
	if (SensorMesh)
	{
		SensorDynamicMaterial = SensorMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (SensorDynamicMaterial)
		{
			SensorDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), SensorColor);
			SensorDynamicMaterial->SetVectorParameterValue(TEXT("Color"), SensorColor);

			if (bIsHitByLaser && bGlowWhenHit)
			{
				SensorDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), ChosenColor * 5.0f);
				SensorDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), ChosenColor * 5.0f);
			}
			else
			{
				SensorDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor::Black);
				SensorDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
			}
		}
	}
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
}
