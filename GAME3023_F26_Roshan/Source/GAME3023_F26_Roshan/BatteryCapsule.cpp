// roshan - 101473963 - game3023 game engines 3
// battery capsule implementation
// handles the mesh, idle bobbing, color change from hex, and attaching to character/cylinder

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
	// need tick enabled for the bobbing/spinning animation
	PrimaryActorTick.bCanEverTick = true;

	// root collision capsule so it doesn't fall through floor
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(35.0f, 55.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	// battery 3d model - scaled cylinder shape for now until we get 3d art
	BatteryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BatteryMesh"));
	BatteryMesh->SetupAttachment(RootComponent);
	BatteryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // mesh has no collision, root capsule handles it
	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));

	// grab basic engine cylinder so we dont need to import fbx
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshFinder.Succeeded())
	{
		BatteryMesh->SetStaticMesh(MeshFinder.Object);
	}

	// flat color material so we can tint it dynamically
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (BaseMatFinder.Succeeded())
	{
		BatteryMesh->SetMaterial(0, BaseMatFinder.Object);
	}

	// trigger zone for pressing E
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(160.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// update color right in editor viewport when changing hex code
	ApplyColorFromHex();
}

void ABatteryCapsule::BeginPlay()
{
	Super::BeginPlay();
	
	// remember where it spawned for reset or respawn
	SpawnOrigin = GetActorLocation();
	BaseLocation = SpawnOrigin;

	// bind overlap events for interaction prompt
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionEndOverlap);

	// set initial material color
	ApplyColorFromHex();
}

FString ABatteryCapsule::GetNormalizedHexCode() const
{
	// clean up user input - strip spaces and hashtag
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
	// fallback if hex is invalid length
	if (CleanHex.Len() < 6)
	{
		CleanHex = TEXT("FF0000"); // default to red
	}
	if (CleanHex.Len() == 6)
	{
		CleanHex += TEXT("FF"); // add full alpha
	}

	// convert hex string to unreal color
	const FColor ParsedColor = FColor::FromHex(CleanHex);
	const FLinearColor LinearColor(ParsedColor);

	// create dynamic material instance if we don't have one yet
	if (!DynamicMaterial)
	{
		DynamicMaterial = BatteryMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	// apply color and emissive glow
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

	// spin and float up and down when sitting on the floor
	// dont animate if player is holding it or if it's plugged into the cylinder
	if (!bIsCarried && !bIsSocketed && bEnableIdleAnimation)
	{
		AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));

		RunningTime += DeltaTime;
		// simple sin wave floating formula from math class
		const float DeltaZ = FMath::Sin(RunningTime * BobFrequency) * BobAmplitude;
		SetActorLocation(BaseLocation + FVector(0.0f, 0.0f, DeltaZ));
	}
}

void ABatteryCapsule::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// ignore if already picked up or socketed
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

	// try to put in player inventory - will fail if player already has 1
	return Player->TryPickupBattery(this);
}

void ABatteryCapsule::OnCollected(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player) return;

	bIsCarried = true;

	// turn off collision so it doesnt block player movement while walking
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// try attaching to spine bone, otherwise stick to hip
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
		// fallback to hip if skeleton doesn't have spine_03 socket
		AttachToComponent(Player->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(-30.0f, 25.0f, 10.0f));
		SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	}

	// make it a bit smaller while on player back
	BatteryMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.55f));
}

void ABatteryCapsule::OnSocketed(AActor* SocketActor, USceneComponent* AttachTargetComp)
{
	bIsCarried = false;
	bIsSocketed = true;

	// detach from player first
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// snap onto the cylinder's battery socket
	if (AttachTargetComp)
	{
		AttachToComponent(AttachTargetComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else if (SocketActor)
	{
		AttachToActor(SocketActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	// zero out transform relative to socket
	SetActorRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	SetActorRelativeScale3D(FVector(0.45f, 0.45f, 0.7f));

	// disable collisions so player can walk up close to cylinder
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABatteryCapsule::OnDropped(const FVector& DropLocation)
{
	bIsCarried = false;
	bIsSocketed = false;

	// detach from player and place on ground
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(DropLocation);
	SetActorRotation(FRotator::ZeroRotator);
	BaseLocation = DropLocation;
	RunningTime = 0.0f; // reset bob timer

	// reset scale and re-enable collisions so player can pick it up again
	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABatteryCapsule::RespawnToOrigin()
{
	bIsCarried = false;

	// if it falls off arena, put it back at spawn
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(SpawnOrigin);
	BaseLocation = SpawnOrigin;

	BatteryMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
