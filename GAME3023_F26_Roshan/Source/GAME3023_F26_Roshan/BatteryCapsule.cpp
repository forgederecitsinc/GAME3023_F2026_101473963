#include "BatteryCapsule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "LiveWireUtils.h"

// Sets up default collision and interaction triggers for the battery capsule
ABatteryCapsule::ABatteryCapsule()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->InitCapsuleSize(45.0f, 92.0f);
	CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CapsuleComponent->SetGenerateOverlapEvents(true);

	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(BatteryPickupRadius);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

// Updates editor visual state and colors when properties change in editor
void ABatteryCapsule::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (InteractionZone)
	{
		InteractionZone->SetSphereRadius(BatteryPickupRadius);
	}
}

#if WITH_EDITOR
// Handles property changes in editor viewport to refresh colors
void ABatteryCapsule::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// Stores spawn position and initializes ID color and collision
void ABatteryCapsule::BeginPlay()
{
	Super::BeginPlay();

	SpawnOrigin = GetActorLocation();
	BaseLocation = SpawnOrigin;

	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ABatteryCapsule::OnInteractionEndOverlap);
}

// Updates battery floating animation or state while dropped
void ABatteryCapsule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsCarried || bIsSocketed || !bEnableIdleAnimation)
	{
		return;
	}

	AddActorLocalRotation(FRotator(0.0f, RotationSpeed * DeltaTime, 0.0f));
	RunningTime += DeltaTime;
	const float DeltaZ = FMath::Sin(RunningTime * BobFrequency) * BobAmplitude;
	SetActorLocation(BaseLocation + FVector(0.0f, 0.0f, DeltaZ));
}

// Shows interaction prompt when player walks into pickup range
void ABatteryCapsule::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsCarried || bIsSocketed)
	{
		return;
	}

	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);
	}
}

// Hides interaction prompt when player leaves pickup range
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

// Picks up or grabs the battery when player presses interact
bool ABatteryCapsule::Interact(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player || bIsCarried || bIsSocketed)
	{
		return false;
	}

	const FVector PlayerLoc = Player->GetActorLocation();
	const FVector BatteryLoc = GetActorLocation();
	if (FVector::DistSquared2D(PlayerLoc, BatteryLoc) > FMath::Square(BatteryPickupRadius) ||
		FMath::Abs(PlayerLoc.Z - BatteryLoc.Z) > 160.0f)
	{
		return false;
	}

	return Player->TryPickupBattery(this);
}

//collecting the object will latch the battery to player's spine
// Attaches battery to player socket and disables world physics
void ABatteryCapsule::OnCollected(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	bIsCarried = true;
	SetAttachmentCollisionEnabled(false);

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
		SetActorRelativeRotation(FRotator::ZeroRotator);
	}

	SetActorRelativeScale3D(FVector(0.35f));
}

// Snaps battery into a cylinder or socket and locks its transform
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

	SetActorRelativeLocation(FVector::ZeroVector);
	SetActorRelativeRotation(FRotator::ZeroRotator);
	SetActorRelativeScale3D(FVector(0.45f));
	SetAttachmentCollisionEnabled(false);
}

// Drops battery onto ground and re-enables collision and prompt triggers
void ABatteryCapsule::OnDropped(const FVector& DropLocation)
{
	ResetToWorldLocation(DropLocation);
}

// Toggles collision profiles on/off when socketing or carrying
void ABatteryCapsule::SetAttachmentCollisionEnabled(bool bEnabled)
{
	SetActorEnableCollision(bEnabled);

	if (bEnabled)
	{
		CapsuleComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		InteractionZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InteractionZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

// Detaches battery and places it at given world coordinates
void ABatteryCapsule::ResetToWorldLocation(const FVector& TargetLocation)
{
	bIsCarried = false;
	bIsSocketed = false;

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocation(TargetLocation);
	SetActorRotation(FRotator::ZeroRotator);
	SetActorRelativeScale3D(FVector::OneVector);

	BaseLocation = TargetLocation;
	RunningTime = 0.0f;

	SetAttachmentCollisionEnabled(true);
}
