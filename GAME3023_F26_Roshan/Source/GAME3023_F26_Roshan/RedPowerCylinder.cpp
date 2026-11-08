// roshan - 101473963 - game3023 game engines 3
// red power cylinder cpp - industrial sharp-edged laser gun turret
// uses 100% sharp-edged cube primitives to match the substation arena blockout
// when a matching battery is attached into the top cradle, fires a bright focused laser beam from the front nozzle

#include "RedPowerCylinder.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "BatteryCapsule.h"
#include "LaserTarget.h"
#include "TransformerBushing.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "GAME3023_F26_Roshan.h"
#include "Kismet/KismetSystemLibrary.h"

ARedPowerCylinder::ARedPowerCylinder()
{
	PrimaryActorTick.bCanEverTick = true;

	// root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// sharp cube primitive used for blockout components
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

	// cylinder primitive used for the upright post column (matching the arena laser turrets)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CylinderMeshObj = CylinderFinder.Succeeded() ? CylinderFinder.Object.Get() : CubeMesh;

	// flat color prototype material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	UMaterialInterface* BaseFlatMat = FlatMatFinder.Succeeded() ? FlatMatFinder.Object : nullptr;

	// dedicated laser beam material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LaserMatFinder(TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	UMaterialInterface* LaserBaseMat = LaserMatFinder.Succeeded() ? LaserMatFinder.Object.Get() : BaseFlatMat;

	// 1. base plinth - sharp dark square slab on floor (300x300x55 cm, Z = 0 to 55)
	BasePlinthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePlinthMesh"));
	BasePlinthMesh->SetupAttachment(RootComponent);
	BasePlinthMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 27.5f));
	BasePlinthMesh->SetRelativeScale3D(FVector(3.0f, 3.0f, 0.55f));
	BasePlinthMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BasePlinthMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) BasePlinthMesh->SetMaterial(0, BaseFlatMat);

	// 2. base pedestal plate - sharp middle tier slab (340x340x26 cm, Z = 55 to 81)
	BasePedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePedestalMesh"));
	BasePedestalMesh->SetupAttachment(RootComponent);
	BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
	BasePedestalMesh->SetRelativeScale3D(FVector(3.4f, 3.4f, 0.26f));
	BasePedestalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BasePedestalMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) BasePedestalMesh->SetMaterial(0, BaseFlatMat);

	// 3. upright pillar post - solid vertical column (90 cm dia, 125 cm height, Z = 81 to 206)
	// connects base pedestal flush to neck collar with zero gap
	PostColumnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PostColumnMesh"));
	PostColumnMesh->SetupAttachment(RootComponent);
	PostColumnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 143.5f));
	PostColumnMesh->SetRelativeScale3D(FVector(0.90f, 0.90f, 1.25f));
	PostColumnMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CylinderMeshObj) PostColumnMesh->SetStaticMesh(CylinderMeshObj);
	if (BaseFlatMat) PostColumnMesh->SetMaterial(0, BaseFlatMat);

	// 4. neck swivel collar - dark collar plate beneath the gun head (220x220x20 cm, Z = 206 to 226)
	NeckCollarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NeckCollarMesh"));
	NeckCollarMesh->SetupAttachment(RootComponent);
	NeckCollarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 216.0f));
	NeckCollarMesh->SetRelativeScale3D(FVector(2.20f, 2.20f, 0.20f));
	NeckCollarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) NeckCollarMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) NeckCollarMesh->SetMaterial(0, BaseFlatMat);

	// 5. main gun receiver body - sharp rectangular box tinted with cell color (200x170x120 cm, Z = 226 to 346)
	// kept named CylinderMesh so existing blueprint child classes and code continue working cleanly
	CylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CylinderMesh"));
	CylinderMesh->SetupAttachment(RootComponent);
	CylinderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 286.0f));
	CylinderMesh->SetRelativeScale3D(FVector(2.00f, 1.70f, 1.20f));
	CylinderMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) CylinderMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) CylinderMesh->SetMaterial(0, BaseFlatMat);

	// 6. front stepped barrel extension - sharp rectangular box (180x80x80 cm, extends forward)
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(RootComponent);
	BarrelMesh->SetRelativeLocation(FVector(180.0f, 0.0f, 286.0f));
	BarrelMesh->SetRelativeScale3D(FVector(1.80f, 0.80f, 0.80f));
	BarrelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BarrelMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) BarrelMesh->SetMaterial(0, BaseFlatMat);

	// 7. front emitter nozzle tip - sharp dark box (50x40x60 cm at the tip of the barrel)
	NozzleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NozzleMesh"));
	NozzleMesh->SetupAttachment(RootComponent);
	NozzleMesh->SetRelativeLocation(FVector(295.0f, 0.0f, 286.0f));
	NozzleMesh->SetRelativeScale3D(FVector(0.50f, 0.40f, 0.60f));
	NozzleMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) NozzleMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) NozzleMesh->SetMaterial(0, BaseFlatMat);

	// 8. battery cradle socket bracket on top deck of the gun body (80x80x10 cm, Z = 346 to 356)
	BatteryDockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryDockMesh"));
	BatteryDockMesh->SetupAttachment(RootComponent);
	BatteryDockMesh->SetRelativeLocation(FVector(-20.0f, 0.0f, 351.0f));
	BatteryDockMesh->SetRelativeScale3D(FVector(0.80f, 0.80f, 0.10f));
	BatteryDockMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BatteryDockMesh->SetStaticMesh(CubeMesh);
	if (BaseFlatMat) BatteryDockMesh->SetMaterial(0, BaseFlatMat);

	// 9. socket where the battery snaps in when inserted (Z = 360, centered in cradle)
	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(-20.0f, 0.0f, 360.0f));

	// 10. laser emitter socket at the front face of the nozzle (X = 322, Z = 286)
	LaserEmitter = CreateDefaultSubobject<USceneComponent>(TEXT("LaserEmitter"));
	LaserEmitter->SetupAttachment(RootComponent);
	LaserEmitter->SetRelativeLocation(FVector(322.0f, 0.0f, 286.0f));
	LaserEmitter->SetRelativeRotation(FRotator::ZeroRotator);

	// 11. visual laser beam mesh - sharp rectangular beam that points forward from the nozzle
	LaserBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeamMesh"));
	LaserBeamMesh->SetupAttachment(LaserEmitter);
	LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamMesh->SetCastShadow(false);
	if (CubeMesh) LaserBeamMesh->SetStaticMesh(CubeMesh);
	if (LaserBaseMat) LaserBeamMesh->SetMaterial(0, LaserBaseMat);
	LaserBeamMesh->SetVisibility(false); // off until battery is attached

	// 12. interaction trigger zone for player E / controller square (focused around upper gun deck)
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	InteractionZone->SetSphereRadius(240.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::BuildGunComponents()
{
	// cube and cylinder primitives
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMeshObj = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* BaseFlatMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	UMaterialInterface* LaserBaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	if (!LaserBaseMat) LaserBaseMat = BaseFlatMat;

	// ensure base plinth exists
	if (!BasePlinthMesh)
	{
		BasePlinthMesh = NewObject<UStaticMeshComponent>(this, TEXT("BasePlinthMesh"));
		if (BasePlinthMesh)
		{
			BasePlinthMesh->RegisterComponent();
			BasePlinthMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (BasePlinthMesh)
	{
		BasePlinthMesh->SetVisibility(bShowBaseStructure);
		BasePlinthMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		BasePlinthMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 27.5f));
		BasePlinthMesh->SetRelativeScale3D(FVector(3.0f, 3.0f, 0.55f));
		BasePlinthMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BasePlinthMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BasePlinthMesh->GetStaticMesh()) BasePlinthMesh->SetStaticMesh(CubeMesh);
		if (BasePlinthMaterial)
		{
			BasePlinthMesh->SetMaterial(0, BasePlinthMaterial);
		}
		else if (BaseFlatMat && !BasePlinthMesh->GetMaterial(0))
		{
			BasePlinthMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// ensure base pedestal plate exists
	if (!BasePedestalMesh)
	{
		BasePedestalMesh = NewObject<UStaticMeshComponent>(this, TEXT("BasePedestalMesh"));
		if (BasePedestalMesh)
		{
			BasePedestalMesh->RegisterComponent();
			BasePedestalMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (BasePedestalMesh)
	{
		BasePedestalMesh->SetVisibility(bShowBaseStructure);
		BasePedestalMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 68.0f));
		BasePedestalMesh->SetRelativeScale3D(FVector(3.4f, 3.4f, 0.26f));
		BasePedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BasePedestalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BasePedestalMesh->GetStaticMesh()) BasePedestalMesh->SetStaticMesh(CubeMesh);
		if (BasePedestalMaterial)
		{
			BasePedestalMesh->SetMaterial(0, BasePedestalMaterial);
		}
		else if (BaseFlatMat && !BasePedestalMesh->GetMaterial(0))
		{
			BasePedestalMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// ensure upright pillar post exists and connects pedestal flush to collar with zero gap
	if (!PostColumnMesh)
	{
		PostColumnMesh = NewObject<UStaticMeshComponent>(this, TEXT("PostColumnMesh"));
		if (PostColumnMesh)
		{
			PostColumnMesh->RegisterComponent();
			PostColumnMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (PostColumnMesh)
	{
		PostColumnMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 143.5f));
		PostColumnMesh->SetRelativeScale3D(FVector(0.90f, 0.90f, 1.25f));
		PostColumnMesh->SetRelativeRotation(FRotator::ZeroRotator);
		PostColumnMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		UStaticMesh* PostMesh = (bUseCylinderPost && CylinderMeshObj) ? CylinderMeshObj : CubeMesh;
		if (PostMesh && !PostColumnMesh->GetStaticMesh()) PostColumnMesh->SetStaticMesh(PostMesh);
		if (PostColumnMaterial)
		{
			PostColumnMesh->SetMaterial(0, PostColumnMaterial);
		}
		else if (BaseFlatMat && !PostColumnMesh->GetMaterial(0))
		{
			PostColumnMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// rotation rotator based on GunYaw for swiveling the gun head
	const FRotator AimRot(0.0f, GunYaw, 0.0f);

	// ensure neck swivel collar exists
	if (!NeckCollarMesh)
	{
		NeckCollarMesh = NewObject<UStaticMeshComponent>(this, TEXT("NeckCollarMesh"));
		if (NeckCollarMesh)
		{
			NeckCollarMesh->RegisterComponent();
			NeckCollarMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (NeckCollarMesh)
	{
		NeckCollarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 216.0f));
		NeckCollarMesh->SetRelativeScale3D(FVector(2.20f, 2.20f, 0.20f));
		NeckCollarMesh->SetRelativeRotation(AimRot);
		NeckCollarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !NeckCollarMesh->GetStaticMesh()) NeckCollarMesh->SetStaticMesh(CubeMesh);
		if (NeckCollarMaterial)
		{
			NeckCollarMesh->SetMaterial(0, NeckCollarMaterial);
		}
		else if (BaseFlatMat && !NeckCollarMesh->GetMaterial(0))
		{
			NeckCollarMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// configure main gun receiver body (sharp box)
	if (CylinderMesh)
	{
		CylinderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 286.0f));
		CylinderMesh->SetRelativeScale3D(FVector(2.00f, 1.70f, 1.20f));
		CylinderMesh->SetRelativeRotation(AimRot);
		CylinderMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !CylinderMesh->GetStaticMesh()) CylinderMesh->SetStaticMesh(CubeMesh);
		if (BodyMaterial)
		{
			CylinderMesh->SetMaterial(0, BodyMaterial);
		}
		else if (BaseFlatMat && !CylinderMesh->GetMaterial(0))
		{
			CylinderMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// ensure front stepped barrel exists
	if (!BarrelMesh)
	{
		BarrelMesh = NewObject<UStaticMeshComponent>(this, TEXT("BarrelMesh"));
		if (BarrelMesh)
		{
			BarrelMesh->RegisterComponent();
			BarrelMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (BarrelMesh)
	{
		const FVector BarrelLoc = AimRot.RotateVector(FVector(180.0f, 0.0f, 0.0f)) + FVector(0.0f, 0.0f, 286.0f);
		BarrelMesh->SetRelativeLocation(BarrelLoc);
		BarrelMesh->SetRelativeScale3D(FVector(1.80f, 0.80f, 0.80f));
		BarrelMesh->SetRelativeRotation(AimRot);
		BarrelMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BarrelMesh->GetStaticMesh()) BarrelMesh->SetStaticMesh(CubeMesh);
		if (BarrelMaterial)
		{
			BarrelMesh->SetMaterial(0, BarrelMaterial);
		}
		else if (BaseFlatMat && !BarrelMesh->GetMaterial(0))
		{
			BarrelMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// ensure front emitter nozzle tip exists
	if (!NozzleMesh)
	{
		NozzleMesh = NewObject<UStaticMeshComponent>(this, TEXT("NozzleMesh"));
		if (NozzleMesh)
		{
			NozzleMesh->RegisterComponent();
			NozzleMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (NozzleMesh)
	{
		const FVector NozzleLoc = AimRot.RotateVector(FVector(295.0f, 0.0f, 0.0f)) + FVector(0.0f, 0.0f, 286.0f);
		NozzleMesh->SetRelativeLocation(NozzleLoc);
		NozzleMesh->SetRelativeScale3D(FVector(0.50f, 0.40f, 0.60f));
		NozzleMesh->SetRelativeRotation(AimRot);
		NozzleMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !NozzleMesh->GetStaticMesh()) NozzleMesh->SetStaticMesh(CubeMesh);
		if (NozzleMaterial)
		{
			NozzleMesh->SetMaterial(0, NozzleMaterial);
		}
		else if (BaseFlatMat && !NozzleMesh->GetMaterial(0))
		{
			NozzleMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// ensure battery cradle socket bracket exists
	if (!BatteryDockMesh)
	{
		BatteryDockMesh = NewObject<UStaticMeshComponent>(this, TEXT("BatteryDockMesh"));
		if (BatteryDockMesh)
		{
			BatteryDockMesh->RegisterComponent();
			BatteryDockMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (BatteryDockMesh)
	{
		const FVector DockLoc = AimRot.RotateVector(FVector(-20.0f, 0.0f, 65.0f)) + FVector(0.0f, 0.0f, 286.0f);
		BatteryDockMesh->SetRelativeLocation(DockLoc);
		BatteryDockMesh->SetRelativeScale3D(FVector(0.80f, 0.80f, 0.10f));
		BatteryDockMesh->SetRelativeRotation(AimRot);
		BatteryDockMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BatteryDockMesh->GetStaticMesh()) BatteryDockMesh->SetStaticMesh(CubeMesh);
		if (BatteryDockMaterial)
		{
			BatteryDockMesh->SetMaterial(0, BatteryDockMaterial);
		}
		else if (BaseFlatMat && !BatteryDockMesh->GetMaterial(0))
		{
			BatteryDockMesh->SetMaterial(0, BaseFlatMat);
		}
	}

	// configure battery socket
	if (BatterySocket)
	{
		const FVector SocketLoc = AimRot.RotateVector(FVector(-20.0f, 0.0f, 75.0f)) + FVector(0.0f, 0.0f, 286.0f);
		BatterySocket->SetRelativeLocation(SocketLoc);
		BatterySocket->SetRelativeRotation(AimRot);
	}

	// configure laser emitter
	if (LaserEmitter)
	{
		const FVector EmitterLoc = AimRot.RotateVector(FVector(322.0f, 0.0f, 0.0f)) + FVector(0.0f, 0.0f, 286.0f);
		LaserEmitter->SetRelativeLocation(EmitterLoc);
		LaserEmitter->SetRelativeRotation(AimRot);
	}

	// configure laser beam mesh (sharp cube!)
	if (LaserBeamMesh)
	{
		if (CubeMesh && !LaserBeamMesh->GetStaticMesh()) LaserBeamMesh->SetStaticMesh(CubeMesh);
		if (LaserBeamMaterial)
		{
			LaserBeamMesh->SetMaterial(0, LaserBeamMaterial);
		}
		else if (LaserBaseMat && !LaserBeamMesh->GetMaterial(0))
		{
			LaserBeamMesh->SetMaterial(0, LaserBaseMat);
		}
		LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LaserBeamMesh->SetCastShadow(false);
	}

	// configure interaction trigger zone
	if (InteractionZone)
	{
		InteractionZone->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
		InteractionZone->SetSphereRadius(240.0f);
	}
}

void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildGunComponents();
	ApplyColorFromID();
	UpdateLaserBeam();
}

#if WITH_EDITOR
void ARedPowerCylinder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplyColorFromID();
	UpdateLaserBeam();
}
#endif

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	BuildGunComponents();
	ApplyColorFromID();
	UpdateLaserBeam();
}

FLinearColor ARedPowerCylinder::GetColorForID(int32 InID) const
{
	// vibrant saturated color palette matching the blockout art in the arena
	switch (InID)
	{
		case 1001: return FLinearColor(1.00f, 0.10f, 0.12f); // vibrant cherry red
		case 1002: return FLinearColor(0.08f, 0.95f, 0.18f); // vibrant electric emerald green
		case 1003: return FLinearColor(0.08f, 0.45f, 1.00f); // vibrant cobalt electric blue
		case 1004: return FLinearColor(1.00f, 0.82f, 0.02f); // vibrant hazard caution yellow (matches arena turrets)
		case 1005: return FLinearColor(1.00f, 0.42f, 0.02f); // vibrant safety neon orange
		case 1006: return FLinearColor(0.78f, 0.10f, 1.00f); // vibrant neon electric violet
		case 1007: return FLinearColor(0.02f, 0.92f, 0.98f); // vibrant bright laser cyan
		case 1008: return FLinearColor(1.00f, 0.10f, 0.65f); // vibrant hot neon magenta
		default:
		{
			// high saturation and high brightness for any custom 4-digit ID
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 240, 255);
		}
	}
}

void ARedPowerCylinder::ApplyColorFromID()
{
	// NEVER create transient dynamic material instances on templates or CDOs in the Blueprint Editor!
	// Creating transient MIDs during Blueprint compilation corrupts component template serialization and reverts materials.
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	const FLinearColor ChosenColor = GetColorForID(CellID);
	const FLinearColor PlinthColor = FLinearColor(0.12f, 0.12f, 0.14f);
	const FLinearColor PedestalColor = FLinearColor(0.82f, 0.82f, 0.85f);
	const FLinearColor PillarColor = FLinearColor(0.80f, 0.82f, 0.86f);
	const FLinearColor NeckCollarColor = FLinearColor(0.14f, 0.14f, 0.16f);
	const FLinearColor NozzleColor = FLinearColor(0.08f, 0.08f, 0.10f);
	const FLinearColor BatteryDockColor = ChosenColor * 0.75f;
	const FLinearColor BodyColor = ChosenColor;
	const FLinearColor BarrelColor = ChosenColor;

	UMaterialInterface* BaseFlatMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));

	auto SetMeshColor = [BaseFlatMat](UStaticMeshComponent* MeshComp, const FLinearColor& BaseCol, const FLinearColor& EmissiveCol, bool bForceTint = false) -> UMaterialInstanceDynamic*
	{
		if (!MeshComp) return nullptr;
		UMaterialInterface* CurrentMat = MeshComp->GetMaterial(0);
		if (!CurrentMat) return nullptr;

		// Check if this is the default prototype flat material
		const bool bIsDefaultFlatMat = (CurrentMat == BaseFlatMat) || CurrentMat->GetName().Contains(TEXT("FlatCol"));

		// If the user assigned a custom material and there is no emissive glow needed, leave it completely untouched!
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

	// 1. base plinth foundation (dark industrial charcoal contrast)
	SetMeshColor(BasePlinthMesh, PlinthColor, FLinearColor::Black);

	// 2. base pedestal plate (clean bright industrial cream/concrete)
	SetMeshColor(BasePedestalMesh, PedestalColor, FLinearColor::Black);

	// 3. upright pillar post (clean bright silver alloy that catches arena light)
	SetMeshColor(PostColumnMesh, PillarColor, FLinearColor::Black);

	// 4. neck swivel collar plate (dark graphite collar with subtle accent tint)
	SetMeshColor(NeckCollarMesh, NeckCollarColor, FLinearColor::Black);

	// 5. front emitter nozzle tip (dark gunmetal aperture with glowing lens mouth when powered)
	const FLinearColor NozzleEmissive = bIsPowered ? (BodyColor * 3.0f) : FLinearColor::Black;
	SetMeshColor(NozzleMesh, NozzleColor, NozzleEmissive);

	// 6. battery dock cradle on top deck (illuminated with dock color accent)
	const FLinearColor DockEmissive = bIsPowered ? (BatteryDockColor * 2.0f) : FLinearColor::Black;
	SetMeshColor(BatteryDockMesh, BatteryDockColor, DockEmissive);

	// 7. main gun receiver body: rich, 100% saturated full color at all times; glowing bright emissive when powered
	const FLinearColor BodyEmissive = bIsPowered ? (BodyColor * 4.0f) : FLinearColor::Black;
	DynamicMaterial = SetMeshColor(CylinderMesh, BodyColor, BodyEmissive, true);

	// 8. front stepped barrel extension: matching full vibrant color; glowing emissive when powered
	const FLinearColor BarrelEmissive = bIsPowered ? (BarrelColor * 3.5f) : FLinearColor::Black;
	BarrelDynamicMaterial = SetMeshColor(BarrelMesh, BarrelColor, BarrelEmissive, true);
}

bool ARedPowerCylinder::CheckActorTargetIDMatch(AActor* ActorToCheck, int32& OutFoundID) const
{
	if (!ActorToCheck) return false;

	// 1. Direct LaserTarget check
	if (ALaserTarget* LaserTarget = Cast<ALaserTarget>(ActorToCheck))
	{
		OutFoundID = LaserTarget->TargetID;
		return (LaserTarget->TargetID == CellID);
	}

	// 2. Direct TransformerBushing check
	if (ATransformerBushing* Bushing = Cast<ATransformerBushing>(ActorToCheck))
	{
		OutFoundID = Bushing->BushingID;
		return (Bushing->BushingID == CellID);
	}

	// 2. Direct other RedPowerCylinder check
	if (ARedPowerCylinder* OtherCyl = Cast<ARedPowerCylinder>(ActorToCheck))
	{
		if (OtherCyl != this)
		{
			OutFoundID = OtherCyl->CellID;
			return (OtherCyl->CellID == CellID);
		}
	}

	// 3. Check attached parent actor (in case a child mesh component was hit)
	if (AActor* ParentActor = ActorToCheck->GetAttachParentActor())
	{
		if (ParentActor != ActorToCheck)
		{
			if (CheckActorTargetIDMatch(ParentActor, OutFoundID))
			{
				return true;
			}
		}
	}

	// 4. Generic reflection: check for integer property named "TargetID" or "CellID"
	if (FIntProperty* TargetIdProp = CastField<FIntProperty>(ActorToCheck->GetClass()->FindPropertyByName(TEXT("TargetID"))))
	{
		OutFoundID = TargetIdProp->GetPropertyValue_InContainer(ActorToCheck);
		return (OutFoundID == CellID);
	}
	if (FIntProperty* CellIdProp = CastField<FIntProperty>(ActorToCheck->GetClass()->FindPropertyByName(TEXT("CellID"))))
	{
		OutFoundID = CellIdProp->GetPropertyValue_InContainer(ActorToCheck);
		return (OutFoundID == CellID);
	}

	return false;
}

void ARedPowerCylinder::RotateGun(float AngleDegrees)
{
	GunYaw = FMath::Fmod(GunYaw + AngleDegrees, 360.0f);
	BuildGunComponents();
	UpdateLaserBeam();
}

void ARedPowerCylinder::UpdateLaserBeam()
{
	if (!LaserBeamMesh || !LaserEmitter) return;

	// laser only turns on once battery is attached and powers on the gun!
	// (or when previewing in editor details panel)
	const bool bShouldBeActive = (bIsPowered && bHasBattery) || (bPreviewLaserInEditor && GetWorld() && !GetWorld()->IsGameWorld());

	if (!bShouldBeActive)
	{
		LaserBeamMesh->SetVisibility(false);
		bIsTargetDetected = false;
		DetectedTarget = nullptr;
		bHasPrintedNoTarget = false;
		bHasPrintedTargetDetected = false;
		if (LastHitLaserTarget)
		{
			LastHitLaserTarget->SetLaserHit(false, this);
			LastHitLaserTarget = nullptr;
		}
		if (LastHitBushing)
		{
			LastHitBushing->SetLaserHit(false, this);
			LastHitBushing = nullptr;
		}
		return;
	}

	LaserBeamMesh->SetVisibility(true);

	const FVector Start = LaserEmitter->GetComponentLocation();
	const FVector Forward = LaserEmitter->GetForwardVector();
	const FVector DesiredEnd = Start + Forward * LaserLength;

	float ActualDistance = LaserLength;
	AActor* HitActor = nullptr;

	// raycast forward so the laser beam stops against obstacles and detects target
	if (GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (InsertedBattery)
		{
			Params.AddIgnoredActor(InsertedBattery);
		}

		// Line trace against Visibility channel (hits static geometry, props, and dynamic actors)
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, DesiredEnd, ECC_Visibility, Params);
		if (bHit)
		{
			HitActor = Hit.GetActor();
			if (bLaserStopsAtObstacles)
			{
				ActualDistance = FMath::Max(15.0f, Hit.Distance);
			}
		}
	}

	// position laser beam: cube mesh center is at 0, so offset forward by half distance along X
	// scale X by distance, scale Y and Z by laser thickness
	LaserBeamMesh->SetRelativeLocation(FVector(ActualDistance * 0.5f, 0.0f, 0.0f));
	LaserBeamMesh->SetRelativeRotation(FRotator::ZeroRotator);
	LaserBeamMesh->SetRelativeScale3D(FVector(ActualDistance / 100.0f, LaserThickness / 100.0f, LaserThickness / 100.0f));

	// Do not create dynamic material on templates in Blueprint editor
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		// create bright emissive laser material
		if (!LaserDynamicMaterial)
		{
			LaserDynamicMaterial = LaserBeamMesh->CreateAndSetMaterialInstanceDynamic(0);
		}

		if (LaserDynamicMaterial)
		{
			const FLinearColor LaserBeamColor = GetColorForID(CellID);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), LaserBeamColor);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), LaserBeamColor);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("Color"), LaserBeamColor);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("Emissive Color"), LaserBeamColor * 20.0f);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), LaserBeamColor * 20.0f);
			LaserDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), LaserBeamColor * 20.0f); // intense laser glow!
		}
	}

	// Target detection by matching Target ID during gameplay
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		// 1. Check if laser beam hits player character and deal laser burn damage
		AGAME3023_F26_RoshanCharacter* HitPlayer = Cast<AGAME3023_F26_RoshanCharacter>(HitActor);
		if (!HitPlayer && HitActor)
		{
			HitPlayer = Cast<AGAME3023_F26_RoshanCharacter>(HitActor->GetAttachParentActor());
		}

		if (HitPlayer && !HitPlayer->IsDead())
		{
			const float DeltaSec = GetWorld()->GetDeltaSeconds();
			PlayerLaserDamageTimer += DeltaSec;
			if (PlayerLaserDamageTimer >= 0.35f)
			{
				PlayerLaserDamageTimer = 0.0f;
				HitPlayer->ApplyDamageToPlayer(10.0f, this);
			}
		}
		else
		{
			PlayerLaserDamageTimer = 0.35f;
		}

		int32 FoundTargetID = -1;
		const bool bHitMatchingTarget = CheckActorTargetIDMatch(HitActor, FoundTargetID);
		bIsTargetDetected = bHitMatchingTarget;
		DetectedTarget = bHitMatchingTarget ? HitActor : nullptr;

		// Manage LaserTarget and TransformerBushing glow transitions
		ALaserTarget* CurrentHitLaserTarget = Cast<ALaserTarget>(HitActor);
		ATransformerBushing* CurrentHitBushing = Cast<ATransformerBushing>(HitActor);
		if (HitActor)
		{
			if (!CurrentHitLaserTarget) CurrentHitLaserTarget = Cast<ALaserTarget>(HitActor->GetAttachParentActor());
			if (!CurrentHitBushing) CurrentHitBushing = Cast<ATransformerBushing>(HitActor->GetAttachParentActor());
		}

		if (LastHitLaserTarget && LastHitLaserTarget != CurrentHitLaserTarget)
		{
			LastHitLaserTarget->SetLaserHit(false, this);
			LastHitLaserTarget = nullptr;
		}

		if (LastHitBushing && LastHitBushing != CurrentHitBushing)
		{
			LastHitBushing->SetLaserHit(false, this);
			LastHitBushing = nullptr;
		}

		if (bHitMatchingTarget)
		{
			if (CurrentHitLaserTarget)
			{
				CurrentHitLaserTarget->SetLaserHit(true, this);
				LastHitLaserTarget = CurrentHitLaserTarget;
			}
			if (CurrentHitBushing)
			{
				CurrentHitBushing->SetLaserHit(true, this);
				LastHitBushing = CurrentHitBushing;
			}

			// Reset the "no target" latch so it can print again if we lose target
			bHasPrintedNoTarget = false;

			// Print once upon detecting the matching target object (displayed for 3 seconds)
			if (!bHasPrintedTargetDetected)
			{
				bHasPrintedTargetDetected = true;

#if WITH_EDITOR
				const FString TargetName = HitActor ? HitActor->GetActorLabel() : TEXT("Target");
#else
				const FString TargetName = HitActor ? HitActor->GetName() : TEXT("Target");
#endif
				const FString Msg = FString::Printf(TEXT("[TARGET MATCH: %d] Laser hit matching target object: %s"), CellID, *TargetName);

				UKismetSystemLibrary::PrintString(this, Msg, true, true, FLinearColor(0.1f, 1.0f, 0.2f), 3.0f);

				UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Detected matching target '%s' with ID %d (printed for 3s)."), *GetName(), CellID, *TargetName, FoundTargetID);
			}
		}
		else
		{
			// Reset the "target detected" latch so it can print again when matching target is acquired
			bHasPrintedTargetDetected = false;

			// Print once when emitting laser but not detecting a matching target
			if (!bHasPrintedNoTarget)
			{
				bHasPrintedNoTarget = true;

				const FString Msg = FString::Printf(TEXT("[LASER EMITTING] Laser beam emitted (ID: %d) - No matching target detected"), CellID);

				UKismetSystemLibrary::PrintString(this, Msg, true, true, FLinearColor(1.0f, 0.85f, 0.1f), 2.5f);

				UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Laser emitted - no matching target detected (printed once)."), *GetName(), CellID);
			}
		}
	}
}

void ARedPowerCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if battery is attached and powered, continuously update laser so it reacts if dynamic actors cross the beam
	if (bIsPowered && bHasBattery)
	{
		UpdateLaserBeam();
	}
}

bool ARedPowerCylinder::CanPlayerInteract(const AActor* InPlayerActor) const
{
	if (!InPlayerActor)
	{
		return false;
	}

	const FVector PlayerLoc = InPlayerActor->GetActorLocation();
	const FVector CylinderLoc = GetActorLocation();

	// 1. Elevation check:
	// In Unreal, player capsule center is ~90-100 units above player feet.
	// Cylinder base origin (Z=0 relative) sits on top of the platform/ledge.
	// If player is on the platform, PlayerLoc.Z >= CylinderLoc.Z + 50.0f.
	// If player is on the lower floor/path beneath the platform, PlayerLoc.Z will be below (CylinderLoc.Z + 20.0f).
	if (PlayerLoc.Z < (CylinderLoc.Z + 20.0f))
	{
		return false;
	}

	// Player shouldn't be way above the cylinder either
	if (PlayerLoc.Z > (CylinderLoc.Z + 400.0f))
	{
		return false;
	}

	// 2. 2D Horizontal distance check: player must be next to the cylinder
	const float Dist2DSq = FVector::DistSquared2D(PlayerLoc, CylinderLoc);
	if (Dist2DSq > FMath::Square(280.0f))
	{
		return false;
	}

	// 3. Line of sight check:
	// Verify that the platform floor or walls do not block the path between the player and cylinder gun
	UWorld* World = GetWorld();
	if (World)
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(InPlayerActor);

		const FVector EyeLoc = PlayerLoc + FVector(0.0f, 0.0f, 40.0f);
		const FVector CylinderTargetLoc = CylinderLoc + FVector(0.0f, 0.0f, 286.0f);

		if (World->LineTraceSingleByChannel(HitResult, EyeLoc, CylinderTargetLoc, ECC_Visibility, QueryParams))
		{
			if (HitResult.bBlockingHit && HitResult.GetActor() && HitResult.GetActor() != this && HitResult.GetActor() != InPlayerActor)
			{
				return false;
			}
		}
	}

	return true;
}

void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (!CanPlayerInteract(Player))
		{
			return;
		}

		Player->SetCurrentInteractable(this);

		if (GEngine)
		{
			if (bHasBattery)
			{
				GEngine->AddOnScreenDebugMessage(
					2001,
					2.5f,
					FColor::Cyan,
					FString::Printf(TEXT("[Laser Gun] Press [Square] to Remove Battery (ID: %d)"), CellID)
				);
			}
			else
			{
				if (Player->HasBattery())
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.5f,
						FColor::Green,
						FString::Printf(TEXT("[Laser Gun] Press [Square] to Attach Battery into Laser Gun (ID: %d)"), CellID)
					);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.5f,
						FColor::Orange,
						FString::Printf(TEXT("[Laser Gun] Laser Gun (ID: %d): OFFLINE (Requires Matching Battery)"), CellID)
					);
				}
			}
		}
	}
}

void ARedPowerCylinder::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (Player->GetCurrentInteractable() == this)
		{
			Player->SetCurrentInteractable(nullptr);
		}
	}
}

bool ARedPowerCylinder::Interact(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player || !CanPlayerInteract(Player)) return false;

	// 1. remove battery if already plugged in
	if (bHasBattery)
	{
		if (Player->HasBattery())
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.5f,
					FColor::Red,
					TEXT("[INVENTORY FULL] Hands full! Press [R] / [Circle] to drop your current battery first."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}
			return false;
		}

		ABatteryCapsule* Extracted = RemoveBattery();
		if (Extracted)
		{
			Player->TryPickupBattery(Extracted);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					4.0f,
					FColor::Yellow,
					FString::Printf(TEXT("[BATTERY REMOVED] Battery removed! Laser Gun (ID: %d) is now OFFLINE."), CellID),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}

			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s (ID: %d): Battery removed. Laser OFF."), *GetName(), CellID);
			return true;
		}
		return false;
	}

	// 2. insert battery into empty laser gun
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		const bool bMatched = InsertBattery(BatteryToSocket);

		if (bMatched)
		{
			Player->TriggerControllerRumble(1.0f, 0.35f, true, true);
		}
		else
		{
			Player->TriggerControllerRumble(0.6f, 0.15f, true, false);
		}

		return bMatched;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.5f,
				FColor::Red,
				FString::Printf(TEXT("[POWER OFFLINE] Laser Gun (ID: %d) needs a matching Battery!"), CellID),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] Laser Gun %s (ID: %d): Interacted without a battery."), *GetName(), CellID);
		return false;
	}
}

ABatteryCapsule* ARedPowerCylinder::RemoveBattery()
{
	if (!bHasBattery || !InsertedBattery) return nullptr;

	ABatteryCapsule* Extracted = InsertedBattery;

	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;
	bIsTargetDetected = false;
	DetectedTarget = nullptr;
	bHasPrintedNoTarget = false;
	bHasPrintedTargetDetected = false;

	if (LastHitLaserTarget)
	{
		LastHitLaserTarget->SetLaserHit(false, this);
		LastHitLaserTarget = nullptr;
	}
	if (LastHitBushing)
	{
		LastHitBushing->SetLaserHit(false, this);
		LastHitBushing = nullptr;
	}

	UpdateVisualState();
	UpdateLaserBeam(); // turns off laser!

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;

	// attach battery to socket on top of the laser gun
	Battery->OnSocketed(this, BatterySocket);

	// 4-digit ID match check: battery id must equal cell id to power on!
	if (Battery->BatteryID == CellID)
	{
		bIsPowered = true;
		bHasPrintedNoTarget = false;
		bHasPrintedTargetDetected = false;
		bIsTargetDetected = false;
		UpdateVisualState(); // glows gun receiver and barrel
		UpdateLaserBeam();   // fires the laser beam out of the nozzle!

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.5f,
				FColor::Green,
				FString::Printf(TEXT("[ID MATCH: %d] Battery attached & Laser Gun ONLINE! Laser beam active!"), CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] Laser Gun %s: ID MATCH (%d == %d). Laser fired!"), *GetName(), CellID, Battery->BatteryID);
		return true;
	}
	else
	{
		// wrong ID: attaches, but stays unpowered and laser stays off
		bIsPowered = false;
		UpdateVisualState();
		UpdateLaserBeam();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Orange,
				FString::Printf(TEXT("[ID MISMATCH] Battery (ID: %d) attached, but Laser Gun (ID: %d) WON'T POWER ON!"), Battery->BatteryID, CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] Laser Gun %s: ID MISMATCH (Battery ID: %d != Cell ID: %d). Attached but laser OFF!"), *GetName(), Battery->BatteryID, CellID);
		return false;
	}
}

void ARedPowerCylinder::UpdateVisualState()
{
	if (bIsPowered && PoweredMaterial)
	{
		if (CylinderMesh) CylinderMesh->SetMaterial(0, PoweredMaterial);
		if (BarrelMesh) BarrelMesh->SetMaterial(0, PoweredMaterial);
		return;
	}
	if (!bIsPowered && UnpoweredMaterial)
	{
		if (CylinderMesh) CylinderMesh->SetMaterial(0, UnpoweredMaterial);
		if (BarrelMesh) BarrelMesh->SetMaterial(0, UnpoweredMaterial);
		return;
	}

	ApplyColorFromID();
}
