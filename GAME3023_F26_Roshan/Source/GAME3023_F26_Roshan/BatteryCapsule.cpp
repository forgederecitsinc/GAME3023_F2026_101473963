// roshan - 101473963 - game3023 game engines 3
// battery capsule cpp
// using 4-digit id instead of hex now, way easier to match with cylinders

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

	// root collision capsule
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(35.0f, 55.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	// battery cylinder mesh
	BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
	BatteryMesh->SetupAttachment(RootComponent);
	BatteryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshFinder.Succeeded())
	{
		BatteryMesh->SetStaticMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (BaseMatFinder.Succeeded())
	{
		BatteryMesh->SetMaterial(0, BaseMatFinder.Object);
	}

	// interaction sphere for E prompt
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(160.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// apply color whenever we change the ID in editor
	ApplyColorFromID();
}

void ABatteryCapsule::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnOrigin = GetActorLocation();
	BaseLocation = SpawnOrigin;

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionEndOverlap);

	ApplyColorFromID();
}

FLinearColor ABatteryCapsule::GetColorForID(int32 InID) const
{
	// handy presets for quick testing in level
	switch (InID)
	{
		case 1001: return FLinearColor(0.95f, 0.1f, 0.1f);  // red
		case 1002: return FLinearColor(0.1f, 0.9f, 0.15f); // green
		case 1003: return FLinearColor(0.1f, 0.45f, 1.0f); // blue
		case 1004: return FLinearColor(1.0f, 0.85f, 0.1f); // yellow
		case 1005: return FLinearColor(1.0f, 0.4f, 0.05f); // orange
		case 1006: return FLinearColor(0.8f, 0.15f, 1.0f); // purple
		case 1007: return FLinearColor(0.1f, 0.9f, 0.9f);  // cyan
		case 1008: return FLinearColor(1.0f, 0.15f, 0.6f); // magenta
		default:
		{
			// for any other 4 digit number, calculate a unique hue from the ID
			// so matching IDs will always share the exact same color!
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 220, 240);
		}
	}
}

void ABatteryCapsule::ApplyColorFromID()
{
	if (!BatteryMesh) return;

	// check if user wants custom color or auto color from ID
	const FLinearColor ChosenColor = bUseCustomColor ? CustomColor : GetColorForID(BatteryID);

	if (!DynamicMaterial)
	{
		DynamicMaterial = BatteryMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ChosenColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ChosenColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), ChosenColor * 2.0f);
	}
}

void ABatteryCapsule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// floating animation on ground
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
				FString::Printf(TEXT("[Live Wire] Press [E] to Collect Battery (ID: %d)"), BatteryID)
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

	BatteryMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.55f));
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
	SetActorRelativeScale3D(FVector(0.45f, 0.45f, 0.7f));

	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABatteryCapsule::OnDropped(const FVector& DropLocation)
{
	bIsCarried = false;
	bIsSocketed = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(DropLocation);
	SetActorRotation(FRotator::ZeroRotator);
	BaseLocation = DropLocation;
	RunningTime = 0.0f;

	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABatteryCapsule::RespawnToOrigin()
{
	bIsCarried = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(SpawnOrigin);
	BaseLocation = SpawnOrigin;

	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
