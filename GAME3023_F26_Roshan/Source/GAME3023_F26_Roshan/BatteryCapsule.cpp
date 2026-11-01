// Fill out your copyright notice in the Description page of Project Settings.

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

	// 1. Physical Collision Capsule
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(35.0f, 55.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	// 2. Battery Mesh (Capsule / Cylinder shape)
	BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
	BatteryMesh->SetupAttachment(RootComponent);
	BatteryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));

	// Default shape from Engine basic shapes
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

	// 3. Interaction Zone (Trigger for player pressing E)
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(160.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColorFromHex();
}

void ABatteryCapsule::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnOrigin = GetActorLocation();
	BaseLocation = SpawnOrigin;

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionEndOverlap);

	ApplyColorFromHex();
}

FString ABatteryCapsule::GetNormalizedHexCode() const
{
	FString Clean = ColorHexCode.TrimStartAndEnd();
	if (Clean.StartsWith(TEXT("#")))
	{
		Clean.RightChopInline(1);
	}
	return Clean.ToUpper();
}

void ABatteryCapsule::ApplyColorFromHex()
{
	if (!BatteryMesh) return;

	FString CleanHex = GetNormalizedHexCode();
	if (CleanHex.Len() < 6)
	{
		CleanHex = TEXT("FF0000"); // default fallback red
	}
	if (CleanHex.Len() == 6)
	{
		CleanHex += TEXT("FF");
	}

	const FColor ParsedColor = FColor::FromHex(CleanHex);
	const FLinearColor LinearColor(ParsedColor);

	if (!DynamicMaterial)
	{
		DynamicMaterial = BatteryMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), LinearColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), LinearColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), LinearColor * 2.0f);
	}
}

void ABatteryCapsule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Idle floating and spinning when placed on ground
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
				TEXT("[Live Wire] Press [E] to Collect Battery Capsule")
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

	// Try collecting into player's single-item inventory
	return Player->TryPickupBattery(this);
}

void ABatteryCapsule::OnCollected(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player) return;

	bIsCarried = true;

	// Disable world collision and interaction while carried
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Attach to Player (spine socket if available, otherwise hip offset)
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
		SetActorRelativeLocation(FVector(-30.0f, 25.0f, 10.0f)); // on back/hip
		SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	}

	// Scale down slightly while carried
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
