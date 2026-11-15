// =============================================================================
// Student: Roshan - 101473963
// Course: GAME3023 - Game Engines 3
// File: GridLaser.cpp
// Description: Implementation of the Security Grid Laser Barrier.
// Uses an Instanced Static Mesh Component to draw all horizontal and vertical
// laser beams in a single draw call for high performance.
// Handles player damage and eliminates Battery Thieves permanently on contact.
// =============================================================================

#include "GridLaser.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "EnemyBatteryThief.h"
#include "GAME3023_F26_RoshanGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

// -----------------------------------------------------------------------------
// CONSTRUCTOR: Sets up scene root, instanced beam meshes, and hazard trigger box
// -----------------------------------------------------------------------------
AGridLaser::AGridLaser()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root Scene Component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Load standard cube shape and laser material
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* LaserMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	if (!LaserMat)
	{
		LaserMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	LaserMaterialAsset = LaserMat;

	// 2. Instanced Static Mesh Component: draws all laser beams efficiently in 1 draw call!
	LaserBeamInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LaserBeamInstances"));
	LaserBeamInstances->SetupAttachment(RootComponent);
	LaserBeamInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Visual beams don't need collision
	LaserBeamInstances->SetCastShadow(false);
	// Visual beam mesh and materials can be customized in BP_GridLaser

	// 3. Hazard Box: detects player/enemies walking into the laser field
	HazardBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardBox"));
	HazardBox->SetupAttachment(RootComponent);
	HazardBox->SetCollisionProfileName(TEXT("Trigger"));
	HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	HazardBox->SetGenerateOverlapEvents(true);
}

// -----------------------------------------------------------------------------
// BEGIN PLAY: Hooks up overlap delegates and builds the laser grid
// -----------------------------------------------------------------------------
void AGridLaser::BeginPlay()
{
	Super::BeginPlay();

	// Bind hazard trigger overlaps
	if (HazardBox)
	{
		HazardBox->OnComponentBeginOverlap.AddDynamic(this, &AGridLaser::OnHazardBeginOverlap);
		HazardBox->OnComponentEndOverlap.AddDynamic(this, &AGridLaser::OnHazardEndOverlap);
	}

	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}

// -----------------------------------------------------------------------------
// ON CONSTRUCTION: Rebuilds beams in editor when placed or moved
// -----------------------------------------------------------------------------
void AGridLaser::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}

#if WITH_EDITOR
// -----------------------------------------------------------------------------
// POST EDIT CHANGE PROPERTY: Live editor update when changing values in Details
// -----------------------------------------------------------------------------
void AGridLaser::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}
#endif

// -----------------------------------------------------------------------------
// REBUILD LASER GRID: Calculates positions for all horizontal & vertical beams
// -----------------------------------------------------------------------------
void AGridLaser::RebuildLaserGrid()
{
	if (!LaserBeamInstances)
	{
		return;
	}

	// Make sure a static mesh is assigned
	if (!LaserBeamInstances->GetStaticMesh())
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh)
		{
			LaserBeamInstances->SetStaticMesh(CubeMesh);
		}
	}

	// Clear out any old beam instances
	LaserBeamInstances->ClearInstances();

	const float ClampedWidth = FMath::Max(20.0f, GridWidth);
	const float ClampedHeight = FMath::Max(20.0f, GridHeight);
	const float ClampedThickness = FMath::Max(1.0f, BeamThickness);

	// 1. Generate Horizontal Beams (spanning across width Y, spaced along height Z)
	if (bEnableHorizontalBeams && NumHorizontalBeams > 0)
	{
		const float StepZ = ClampedHeight / (NumHorizontalBeams + 1.0f);
		for (int32 i = 0; i < NumHorizontalBeams; ++i)
		{
			const float ZPos = StepZ * (i + 1.0f);
			FTransform BeamTransform;
			BeamTransform.SetLocation(FVector(0.0f, 0.0f, ZPos));
			BeamTransform.SetRotation(FQuat::Identity);
			BeamTransform.SetScale3D(FVector(ClampedThickness / 100.0f, ClampedWidth / 100.0f, ClampedThickness / 100.0f));
			LaserBeamInstances->AddInstance(BeamTransform);
		}
	}

	// 2. Generate Vertical Beams (spanning from top to bottom Z, spaced across width Y)
	if (bEnableVerticalBeams && NumVerticalBeams > 0)
	{
		const float StepY = ClampedWidth / (NumVerticalBeams + 1.0f);
		for (int32 j = 0; j < NumVerticalBeams; ++j)
		{
			const float YPos = (-ClampedWidth * 0.5f) + (StepY * (j + 1.0f));
			FTransform BeamTransform;
			BeamTransform.SetLocation(FVector(0.0f, YPos, ClampedHeight * 0.5f));
			BeamTransform.SetRotation(FQuat::Identity);
			BeamTransform.SetScale3D(FVector(ClampedThickness / 100.0f, ClampedThickness / 100.0f, ClampedHeight / 100.0f));
			LaserBeamInstances->AddInstance(BeamTransform);
		}
	}

	// 3. Update the Hazard Trigger Box to span the exact opening dimensions
	if (HazardBox)
	{
		const float BoxThickness = FMath::Max(20.0f, ClampedThickness * 2.0f);
		HazardBox->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedHeight * 0.5f));
		HazardBox->SetBoxExtent(FVector(BoxThickness * 0.5f, ClampedWidth * 0.5f, ClampedHeight * 0.5f));
	}
}

// -----------------------------------------------------------------------------
// UPDATE LASER VISUALS: Pushes color and emissive glow to dynamic material
// -----------------------------------------------------------------------------
void AGridLaser::UpdateLaserVisuals()
{
	// Materials assigned in BP_GridLaser are preserved without being overridden in C++
}

// -----------------------------------------------------------------------------
// UPDATE LASER STATE: Turns visibility and collision on/off based on bIsLaserOn
// -----------------------------------------------------------------------------
void AGridLaser::UpdateLaserState()
{
	// 1. Show or hide visual beams
	if (LaserBeamInstances)
	{
		LaserBeamInstances->SetVisibility(bIsLaserOn);
	}

	// 2. Configure hazard collision
	if (HazardBox)
	{
		if (bIsLaserOn)
		{
			if (bBlockPlayer)
			{
				// Physically block player like a solid energy wall
				HazardBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
				HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			else
			{
				// Hazardous trigger: walk through and take damage
				HazardBox->SetCollisionProfileName(TEXT("Trigger"));
				HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
				HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			}

			// If actors are already inside when the lasers turn on, damage them immediately!
			if (GetWorld() && GetWorld()->IsGameWorld())
			{
				TArray<AActor*> OverlappingActors;
				HazardBox->GetOverlappingActors(OverlappingActors);
				for (AActor* Act : OverlappingActors)
				{
					if (Act && Act != this)
					{
						DealLaserDamage(Act);
					}
				}
			}
		}
		else
		{
			// Lasers are OFF: disable collision so everyone can walk through safely
			HazardBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			OverlappingPlayer = nullptr;
			OverlapDamageTimer = 0.0f;
		}
	}
}

// -----------------------------------------------------------------------------
// SET LASER STATE: Turns the laser grid on or off at runtime
// -----------------------------------------------------------------------------
void AGridLaser::SetLaserState(bool bNewActive)
{
	if (bIsLaserOn != bNewActive)
	{
		bIsLaserOn = bNewActive;
		UpdateLaserState();
		OnLaserStateChanged.Broadcast(this, bIsLaserOn);
	}
}

// -----------------------------------------------------------------------------
// TOGGLE LASER: Swaps between ON and OFF
// -----------------------------------------------------------------------------
void AGridLaser::ToggleLaser()
{
	SetLaserState(!bIsLaserOn);
}

// -----------------------------------------------------------------------------
// DEAL LASER DAMAGE: Damages player or permanently eliminates Battery Thief
// -----------------------------------------------------------------------------
void AGridLaser::DealLaserDamage(AActor* TargetActor)
{
	if (!TargetActor || !bIsLaserOn)
	{
		return;
	}

	// 1. Check if the player touched the laser
	AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetActor);
	if (!PlayerChar)
	{
		PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetActor->GetAttachParentActor());
	}

	if (PlayerChar && !PlayerChar->IsDead())
	{
		// 💥 Deals configured DamageAmount (10 points) to player's health
		PlayerChar->ApplyDamageToPlayer(DamageAmount, this);
		return;
	}

	// 2. Check if a Battery Thief enemy touched the laser
	AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(TargetActor);
	if (!Thief)
	{
		Thief = Cast<AEnemyBatteryThief>(TargetActor->GetAttachParentActor());
	}

	if (Thief && !Thief->bIsDead)
	{
		// 💥 Battery Thief dies permanently (drops battery safely and does not respawn)
		Thief->KillPermanently();
		return;
	}

	// Fallback for any other actor with standard damage
	UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, GetInstigatorController(), this, nullptr);
}

// -----------------------------------------------------------------------------
// ON HAZARD BEGIN OVERLAP: Called the instant something enters the laser field
// -----------------------------------------------------------------------------
void AGridLaser::OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsLaserOn || !OtherActor || OtherActor == this)
	{
		return;
	}

	// Handle Player entering
	AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor);
	if (!PlayerChar)
	{
		PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor->GetAttachParentActor());
	}

	if (PlayerChar)
	{
		OverlappingPlayer = PlayerChar;
		OverlapDamageTimer = 0.0f;
		DealLaserDamage(PlayerChar);
		return;
	}

	// Handle Battery Thief entering
	AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(OtherActor);
	if (!Thief)
	{
		Thief = Cast<AEnemyBatteryThief>(OtherActor->GetAttachParentActor());
	}

	if (Thief)
	{
		DealLaserDamage(Thief);
		return;
	}
}

// -----------------------------------------------------------------------------
// ON HAZARD END OVERLAP: Called when something exits the laser field
// -----------------------------------------------------------------------------
void AGridLaser::OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
	}
}

// -----------------------------------------------------------------------------
// TICK: Continuously burns the player if they stand inside an active laser
// -----------------------------------------------------------------------------
void AGridLaser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If player is continuously standing inside the laser grid, tick damage every 0.5s
	if (bIsLaserOn && OverlappingPlayer)
	{
		OverlapDamageTimer += DeltaTime;
		if (OverlapDamageTimer >= DamageTickInterval)
		{
			OverlapDamageTimer = 0.0f;
			DealLaserDamage(OverlappingPlayer);
		}
	}
}
