#include "GridLaser.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "EnemyBatteryThief.h"
#include "LiveWireUtils.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

// Constructor: creates the main components used by the laser grid.
AGridLaser::AGridLaser()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create the root component and attach the other components to it.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// This component stores and draws all of the individual laser beams.
	LaserBeamInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LaserBeamInstances"));
	LaserBeamInstances->SetupAttachment(RootComponent);
	LaserBeamInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamInstances->SetCastShadow(false);

	// This box is used to check for overlap with the player and other actors.
	HazardBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardBox"));
	HazardBox->SetupAttachment(RootComponent);
	HazardBox->SetCollisionProfileName(TEXT("Trigger"));
	HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	HazardBox->SetGenerateOverlapEvents(true);
}

// BeginPlay: runs once when the game starts and connects the overlap events.
void AGridLaser::BeginPlay()
{
	Super::BeginPlay();

	// Register functions that run when an actor enters or leaves the hazard box.
	if (HazardBox)
	{
		HazardBox->OnComponentBeginOverlap.AddDynamic(this, &AGridLaser::OnHazardBeginOverlap);
		HazardBox->OnComponentEndOverlap.AddDynamic(this, &AGridLaser::OnHazardEndOverlap);
	}

	// Build the beams and set the laser's starting state.
	RebuildLaserGrid();
	UpdateLaserState();
}

// OnConstruction: rebuilds the laser when the actor is changed in the editor.
void AGridLaser::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLaserGrid();
	UpdateLaserState();
}

#if WITH_EDITOR
// PostEditChangeProperty: updates the laser after an editor property is changed.
void AGridLaser::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLaserGrid();
	UpdateLaserState();
}
#endif

// RebuildLaserGrid: creates the beams and sizes the overlap box from the settings.
void AGridLaser::RebuildLaserGrid()
{
	if (!LaserBeamInstances)
	{
		return;
	}

	// Use the Unreal engine cube as the beam mesh if no custom mesh is assigned.
	if (!LaserBeamInstances->GetStaticMesh())
	{
		if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			LaserBeamInstances->SetStaticMesh(CubeMesh);
		}
	}

	// Start with no beams because the grid will be rebuilt from the current settings.
	LaserBeamInstances->ClearInstances();

	const float ClampedWidth = FMath::Max(20.0f, GridWidth);
	const float ClampedHeight = FMath::Max(20.0f, GridHeight);
	const float ClampedThickness = FMath::Max(1.0f, BeamThickness);

	// Add horizontal beams by spacing them evenly up the height of the grid.
	if (bEnableHorizontalBeams && NumHorizontalBeams > 0)
	{
		const float StepZ = ClampedHeight / (NumHorizontalBeams + 1.0f);
		const FVector Scale(ClampedThickness / 100.0f, ClampedWidth / 100.0f, ClampedThickness / 100.0f);
		for (int32 i = 0; i < NumHorizontalBeams; ++i)
		{
			LaserBeamInstances->AddInstance(FTransform(FQuat::Identity, FVector(0.0f, 0.0f, StepZ * (i + 1.0f)), Scale));
		}
	}

	// Add vertical beams by spacing them evenly across the width of the grid.
	if (bEnableVerticalBeams && NumVerticalBeams > 0)
	{
		const float StepY = ClampedWidth / (NumVerticalBeams + 1.0f);
		const FVector Scale(ClampedThickness / 100.0f, ClampedThickness / 100.0f, ClampedHeight / 100.0f);
		for (int32 j = 0; j < NumVerticalBeams; ++j)
		{
			const float YPos = (-ClampedWidth * 0.5f) + (StepY * (j + 1.0f));
			LaserBeamInstances->AddInstance(FTransform(FQuat::Identity, FVector(0.0f, YPos, ClampedHeight * 0.5f), Scale));
		}
	}

	// Resize the collision box to match the size of the complete laser grid.
	if (HazardBox)
	{
		const float BoxThickness = FMath::Max(20.0f, ClampedThickness * 2.0f);
		HazardBox->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedHeight * 0.5f));
		HazardBox->SetBoxExtent(FVector(BoxThickness * 0.5f, ClampedWidth * 0.5f, ClampedHeight * 0.5f));
	}
}

// ConfigureHazardCollision: turns collision on or off and enables blocking if needed.
void AGridLaser::ConfigureHazardCollision()
{
	if (!HazardBox)
	{
		return;
	}

	// If the laser is off, disable collision and clear the player overlap state.
	if (!bIsLaserOn)
	{
		HazardBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
		return;
	}

	// Blocking mode stops the player from passing through the laser.
	if (bBlockPlayer)
	{
		HazardBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		// Normal mode only detects overlaps and does not physically block movement.
		HazardBox->SetCollisionProfileName(TEXT("Trigger"));
		HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
		HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// Check for actors already inside the box after the game starts.
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		TArray<AActor*> OverlappingActors;
		HazardBox->GetOverlappingActors(OverlappingActors);
		for (AActor* Actor : OverlappingActors)
		{
			if (Actor && Actor != this)
			{
				DealLaserDamage(Actor);
			}
		}
	}
}

// UpdateLaserState: updates the beam visibility and hazard collision settings.
void AGridLaser::UpdateLaserState()
{
	if (LaserBeamInstances)
	{
		LaserBeamInstances->SetVisibility(bIsLaserOn);
	}
	ConfigureHazardCollision();
}

// DealLaserDamage: applies the correct damage or death behavior to an actor.
void AGridLaser::DealLaserDamage(AActor* TargetActor)
{
	if (!TargetActor || !bIsLaserOn)
	{
		return;
	}

	// If the target is the player, apply the configured laser damage.
	if (AGAME3023_F26_RoshanCharacter* PlayerChar = LiveWireUtils::ResolveActorOrParent<AGAME3023_F26_RoshanCharacter>(TargetActor))
	{
		if (!PlayerChar->IsDead())//if the player is not dead, apply damage
		{
			PlayerChar->ApplyDamageToPlayer(DamageAmount, this); //this is the laser grid that is dealing damage
		}
		return;
	}

	// Battery thieves are destroyed immediately when hit by the laser.
	if (AEnemyBatteryThief* Thief = LiveWireUtils::ResolveActorOrParent<AEnemyBatteryThief>(TargetActor))
	{
		if (!Thief->bIsDead) //if the thief is not dead, kill it permanently
		{
			Thief->KillPermanently();
		}
		return;
	}

	// Other actors use Unreal's standard damage system.
	UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, GetInstigatorController(), this, nullptr);
}

// OnHazardBeginOverlap: runs when an actor starts overlapping the laser.
void AGridLaser::OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsLaserOn || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Remember the player so it can receive damage again while remaining inside.
	if (AGAME3023_F26_RoshanCharacter* PlayerChar = LiveWireUtils::ResolveActorOrParent<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		OverlappingPlayer = PlayerChar;
		OverlapDamageTimer = 0.0f;
		DealLaserDamage(PlayerChar);
		return;
	}

	// Kill a battery thief when it enters the hazard.
	if (AEnemyBatteryThief* Thief = LiveWireUtils::ResolveActorOrParent<AEnemyBatteryThief>(OtherActor))
	{
		DealLaserDamage(Thief);
	}
}

// OnHazardEndOverlap: runs when an actor leaves the laser hazard.
void AGridLaser::OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
	}
}

// Tick: applies damage again while the player stays inside the laser.
void AGridLaser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLaserOn || !OverlappingPlayer)
	{
		return;
	}

	// Count the time spent overlapping, then use the damage interval to deal damage.
	OverlapDamageTimer += DeltaTime;
	if (OverlapDamageTimer >= DamageTickInterval)
	{
		OverlapDamageTimer = 0.0f;
		DealLaserDamage(OverlappingPlayer);
	}
}
