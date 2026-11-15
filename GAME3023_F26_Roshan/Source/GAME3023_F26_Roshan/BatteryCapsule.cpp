#include "BatteryCapsule.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "LiveWireUtils.h"

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
	return LiveWireUtils::GetColorForID(InID);
}

void ABatteryCapsule::ApplyColorFromID()
{
}

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

void ABatteryCapsule::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsCarried || bIsSocketed)
	{
		return;
	}

	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);
		LiveWireUtils::ShowScreenMessage(1001, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("[Live Wire] Press [Square] to Collect Battery (ID: %d)"), BatteryID));
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

void ABatteryCapsule::OnDropped(const FVector& DropLocation)
{
	ResetToWorldLocation(DropLocation);
}

void ABatteryCapsule::RespawnToOrigin()
{
	ResetToWorldLocation(SpawnOrigin);
}

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
