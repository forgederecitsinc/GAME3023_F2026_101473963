// roshan - 101473963 - game3023 game engines 3
// battery capsule cpp - redesigned as an industrial power cell battery
// matches the arena blockout style with dark foundation plinth, cream pedestal, colored cylinder core, and dark top terminal cap
// when collected by player or socketed into red power cylinder, scales and snaps cleanly

#include "BatteryCapsule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GAME3023_F26_RoshanCharacter.h"

ABatteryCapsule::ABatteryCapsule()
{
	PrimaryActorTick.bCanEverTick = true;

	// root collision capsule enclosing the entire battery assembly
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(45.0f, 92.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	// load basic blockout shapes
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CylinderMesh = CylinderMeshFinder.Succeeded() ? CylinderMeshFinder.Object : nullptr;

	// load level prototyping base flat material
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FlatMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	UMaterialInterface* FlatMat = FlatMatFinder.Succeeded() ? FlatMatFinder.Object : nullptr;

	// 1. base plinth foundation - sharp dark charcoal square slab at the bottom (140x140x20 cm)
	BasePlinthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePlinthMesh"));
	BasePlinthMesh->SetupAttachment(RootComponent);
	BasePlinthMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
	BasePlinthMesh->SetRelativeScale3D(FVector(1.40f, 1.40f, 0.20f));
	BasePlinthMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BasePlinthMesh->SetStaticMesh(CubeMesh);
	if (FlatMat) BasePlinthMesh->SetMaterial(0, FlatMat);

	// 2. base pedestal plate - sharp light cream square slab (115x115x30 cm)
	BasePedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasePedestalMesh"));
	BasePedestalMesh->SetupAttachment(RootComponent);
	BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -55.0f));
	BasePedestalMesh->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.30f));
	BasePedestalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BasePedestalMesh->SetStaticMesh(CubeMesh);
	if (FlatMat) BasePedestalMesh->SetMaterial(0, FlatMat);

	// 3. central battery cylinder cell core (80 cm dia, 140 cm height)
	BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
	BatteryMesh->SetupAttachment(RootComponent);
	BatteryMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BatteryMesh->SetRelativeScale3D(FVector(0.80f, 0.80f, 1.40f));
	BatteryMesh->SetCollisionProfileName(TEXT("NoCollision"));
	if (CylinderMesh) BatteryMesh->SetStaticMesh(CylinderMesh);
	if (FlatMat) BatteryMesh->SetMaterial(0, FlatMat);

	// 4. top terminal cap - sharp dark charcoal square cap on top of cylinder (95x95x24 cm)
	TopCapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopCapMesh"));
	TopCapMesh->SetupAttachment(RootComponent);
	TopCapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
	TopCapMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 0.24f));
	TopCapMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) TopCapMesh->SetStaticMesh(CubeMesh);
	if (FlatMat) TopCapMesh->SetMaterial(0, FlatMat);

	// 5. interaction trigger zone for collecting battery with E key
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(180.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildBatteryComponents();
	ApplyColorFromID();
}

#if WITH_EDITOR
void ABatteryCapsule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	BuildBatteryComponents();
	ApplyColorFromID();
}
#endif

void ABatteryCapsule::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnOrigin = GetActorLocation();
	BaseLocation = SpawnOrigin;

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionEndOverlap);

	BuildBatteryComponents();
	ApplyColorFromID();
}

void ABatteryCapsule::BuildBatteryComponents()
{
	// load primitives dynamically for existing level or blueprint instances
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* FlatMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));

	// ensure base plinth mesh exists
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
		BasePlinthMesh->SetVisibility(bShowBaseStructure && !bIsCarried && !bIsSocketed);
		BasePlinthMesh->SetCollisionEnabled((bShowBaseStructure && !bIsCarried && !bIsSocketed) ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		BasePlinthMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -80.0f));
		BasePlinthMesh->SetRelativeScale3D(FVector(1.40f, 1.40f, 0.20f));
		BasePlinthMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BasePlinthMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BasePlinthMesh->GetStaticMesh()) BasePlinthMesh->SetStaticMesh(CubeMesh);
		if (PlinthMaterial) BasePlinthMesh->SetMaterial(0, PlinthMaterial);
		else if (FlatMat && !BasePlinthMesh->GetMaterial(0)) BasePlinthMesh->SetMaterial(0, FlatMat);
	}

	// ensure base pedestal mesh exists
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
		BasePedestalMesh->SetVisibility(bShowBaseStructure && !bIsCarried && !bIsSocketed);
		BasePedestalMesh->SetCollisionEnabled((bShowBaseStructure && !bIsCarried && !bIsSocketed) ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		BasePedestalMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -55.0f));
		BasePedestalMesh->SetRelativeScale3D(FVector(1.15f, 1.15f, 0.30f));
		BasePedestalMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BasePedestalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !BasePedestalMesh->GetStaticMesh()) BasePedestalMesh->SetStaticMesh(CubeMesh);
		if (PedestalMaterial) BasePedestalMesh->SetMaterial(0, PedestalMaterial);
		else if (FlatMat && !BasePedestalMesh->GetMaterial(0)) BasePedestalMesh->SetMaterial(0, FlatMat);
	}

	// configure battery cylinder core
	if (BatteryMesh)
	{
		BatteryMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		BatteryMesh->SetRelativeScale3D(FVector(0.80f, 0.80f, 1.40f));
		BatteryMesh->SetRelativeRotation(FRotator::ZeroRotator);
		BatteryMesh->SetCollisionProfileName(TEXT("NoCollision"));
		if (CylinderMesh && !BatteryMesh->GetStaticMesh()) BatteryMesh->SetStaticMesh(CylinderMesh);
		if (FlatMat && !BatteryMesh->GetMaterial(0)) BatteryMesh->SetMaterial(0, FlatMat);
	}

	// ensure top terminal cap mesh exists
	if (!TopCapMesh)
	{
		TopCapMesh = NewObject<UStaticMeshComponent>(this, TEXT("TopCapMesh"));
		if (TopCapMesh)
		{
			TopCapMesh->RegisterComponent();
			TopCapMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}
	if (TopCapMesh)
	{
		TopCapMesh->SetVisibility(bShowTopCap);
		TopCapMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
		TopCapMesh->SetRelativeScale3D(FVector(0.95f, 0.95f, 0.24f));
		TopCapMesh->SetRelativeRotation(FRotator::ZeroRotator);
		TopCapMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		if (CubeMesh && !TopCapMesh->GetStaticMesh()) TopCapMesh->SetStaticMesh(CubeMesh);
		if (TopCapMaterial) TopCapMesh->SetMaterial(0, TopCapMaterial);
		else if (FlatMat && !TopCapMesh->GetMaterial(0)) TopCapMesh->SetMaterial(0, FlatMat);
	}
}

FLinearColor ABatteryCapsule::GetColorForID(int32 InID) const
{
	// distinct vibrant color palettes matching red power cylinder IDs
	switch (InID)
	{
		case 1001: return FLinearColor(0.95f, 0.10f, 0.10f); // red (Center Yard)
		case 1002: return FLinearColor(0.10f, 0.85f, 0.20f); // forest green (Catwalk East / East Yard)
		case 1003: return FLinearColor(0.10f, 0.50f, 1.00f); // blue (North Yard)
		case 1004: return FLinearColor(1.00f, 0.85f, 0.10f); // yellow (Catwalk West)
		case 1005: return FLinearColor(1.00f, 0.45f, 0.05f); // orange
		case 1006: return FLinearColor(0.75f, 0.20f, 1.00f); // purple (Control Room)
		case 1007: return FLinearColor(0.10f, 0.90f, 0.90f); // cyan / teal
		case 1008: return FLinearColor(1.00f, 0.15f, 0.60f); // magenta
		default:
		{
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 220, 240);
		}
	}
}

void ABatteryCapsule::ApplyColorFromID()
{
	const FLinearColor ChosenColor = GetColorForID(BatteryID);
	UMaterialInterface* FlatMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));

	auto SetMeshColor = [FlatMat](UStaticMeshComponent* MeshComp, const FLinearColor& BaseCol, const FLinearColor& EmissiveCol, UMaterialInterface* CustomMat) -> UMaterialInstanceDynamic*
	{
		if (!MeshComp) return nullptr;
		if (CustomMat)
		{
			MeshComp->SetMaterial(0, CustomMat);
			return nullptr;
		}

		UMaterialInstanceDynamic* Dyn = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0));
		if (!Dyn)
		{
			if (FlatMat)
			{
				Dyn = UMaterialInstanceDynamic::Create(FlatMat, MeshComp);
				MeshComp->SetMaterial(0, Dyn);
			}
			else
			{
				Dyn = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
			}
		}

		if (Dyn)
		{
			Dyn->SetVectorParameterValue(TEXT("BaseColor"), BaseCol);
			Dyn->SetVectorParameterValue(TEXT("Color"), BaseCol);
			Dyn->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveCol);
			Dyn->SetVectorParameterValue(TEXT("Emissive"), EmissiveCol);
		}
		return Dyn;
	};

	// 1. Central battery core (glowing cyan energy)
	DynamicMaterial = SetMeshColor(BatteryMesh, ChosenColor, ChosenColor * 3.0f, nullptr);

	// 2. Base plinth foundation (#383F4C)
	SetMeshColor(BasePlinthMesh, PlinthColor, FLinearColor::Black, PlinthMaterial);

	// 3. Base pedestal slab (#1A1D24)
	SetMeshColor(BasePedestalMesh, PedestalColor, FLinearColor::Black, PedestalMaterial);

	// 4. Top terminal cap (#222730)
	SetMeshColor(TopCapMesh, TopCapColor, FLinearColor::Black, TopCapMaterial);
}

void ABatteryCapsule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// idle bobbing and rotation animation when resting in level
	if (!bIsCarried && !bIsSocketed && bEnableIdleAnimation)
	{
		AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

		RunningTime += DeltaTime;
		const float DeltaZ = FMath::Sin(RunningTime * BobFrequency) * BobAmplitude;
		SetActorLocation(BaseLocation + FVector(0.0f, 0.0f, DeltaZ));
	}
}

void ABatteryCapsule::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsCarried || bIsSocketed) return;

	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				1001,
				2.0f,
				FColor::Cyan,
				FString::Printf(TEXT("[Live Wire] Press [Square] to Collect Battery (ID: %d)"), BatteryID)
			);
		}
	}
}

void ABatteryCapsule::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (Player->GetCurrentInteractable() == this)
		{
			Player->SetCurrentInteractable(nullptr);
		}
	}
}

bool ABatteryCapsule::Interact(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player || bIsCarried || bIsSocketed)
	{
		return false;
	}

	return Player->TryPickupBattery(this);
}

void ABatteryCapsule::OnCollected(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player) return;

	bIsCarried = true;

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (BasePlinthMesh) BasePlinthMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (BasePedestalMesh) BasePedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (TopCapMesh) TopCapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// hide floor dock and pedestal when carried on backpack so player carries the sleek battery cell
	if (bHideBaseWhenCarried)
	{
		if (BasePlinthMesh) BasePlinthMesh->SetVisibility(false);
		if (BasePedestalMesh) BasePedestalMesh->SetVisibility(false);
	}

	USkeletalMeshComponent* CharacterMesh = Player->GetMesh();
	const FName SpineSocket(TEXT("spine_03"));

	if (CharacterMesh && CharacterMesh->DoesSocketExist(SpineSocket))
	{
		AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SpineSocket);
		SetActorRelativeLocation(FVector(0.0f, -20.0f, 0.0f));
		SetActorRelativeRotation(FRotator(0.0f, 90.0f, 90.0f));
	}
	else
	{
		AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(-30.0f, 25.0f, 10.0f));
		SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	}

	// scale full assembly proportionally for backpack carrying
	SetActorRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
}

void ABatteryCapsule::OnSocketed(AActor* SocketActor, USceneComponent* AttachTargetComp)
{
	bIsCarried = false;
	bIsSocketed = true;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	if (AttachTargetComp)
	{
		AttachToComponent(AttachTargetComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else if (SocketActor)
	{
		AttachToActor(SocketActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	SetActorRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// scale full assembly proportionally for socket docking
	SetActorRelativeScale3D(FVector(0.45f, 0.45f, 0.45f));

	// hide floor dock and pedestal when inserted into turret cradle
	if (bHideBaseWhenCarried)
	{
		if (BasePlinthMesh) BasePlinthMesh->SetVisibility(false);
		if (BasePedestalMesh) BasePedestalMesh->SetVisibility(false);
	}

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (BasePlinthMesh) BasePlinthMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (BasePedestalMesh) BasePedestalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (TopCapMesh) TopCapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABatteryCapsule::OnDropped(const FVector& DropLocation)
{
	bIsCarried = false;
	bIsSocketed = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(DropLocation);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BaseLocation = DropLocation;
	RunningTime = 0.0f;

	if (BasePlinthMesh)
	{
		BasePlinthMesh->SetVisibility(bShowBaseStructure);
		BasePlinthMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (BasePedestalMesh)
	{
		BasePedestalMesh->SetVisibility(bShowBaseStructure);
		BasePedestalMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (TopCapMesh)
	{
		TopCapMesh->SetVisibility(bShowTopCap);
		TopCapMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABatteryCapsule::RespawnToOrigin()
{
	bIsCarried = false;
	bIsSocketed = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(SpawnOrigin);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BaseLocation = SpawnOrigin;
	RunningTime = 0.0f;

	if (BasePlinthMesh)
	{
		BasePlinthMesh->SetVisibility(bShowBaseStructure);
		BasePlinthMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (BasePedestalMesh)
	{
		BasePedestalMesh->SetVisibility(bShowBaseStructure);
		BasePedestalMesh->SetCollisionEnabled(bShowBaseStructure ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	}
	if (TopCapMesh)
	{
		TopCapMesh->SetVisibility(bShowTopCap);
		TopCapMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
