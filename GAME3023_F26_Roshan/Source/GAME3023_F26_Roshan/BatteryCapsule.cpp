// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: BatteryCapsule.cpp
// Description: Implementation of the Battery Capsule actor.
// Handles player pickup, backpack attachment, socketing into laser cylinders,
// dropping onto the floor, and idle bob/spin animation.

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

	// Enable idle floating & spinning animation by default
	bEnableIdleAnimation = true;

	// root collision capsule enclosing the entire battery assembly
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(45.0f, 92.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	// interaction trigger zone for collecting battery with E key
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(180.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyColorFromID();
}

#if WITH_EDITOR
void ABatteryCapsule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

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

	ApplyColorFromID();
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
	// We do NOT color or override static meshes in C++!
	// Any materials assigned by the student in the Blueprint (BP_BatteryCapsule) or on the mesh components
	// are preserved exactly as they are without being overwritten or tinted by C++.
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

	// Disable collision on the entire actor (including any Blueprint-added mesh components)
	// so attached meshes cannot collide with or push the player's capsule.
	SetActorEnableCollision(false);
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

	SetActorEnableCollision(false);
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
	SetActorRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	BaseLocation = DropLocation;
	RunningTime = 0.0f;

	SetActorEnableCollision(true);
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

	SetActorEnableCollision(true);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}