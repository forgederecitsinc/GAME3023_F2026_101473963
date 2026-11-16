#include "GridLaser.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "EnemyBatteryThief.h"
#include "LiveWireUtils.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

// Sets up beam mesh, box trigger volume, and audio components
AGridLaser::AGridLaser()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LaserBeamInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LaserBeamInstances"));
	LaserBeamInstances->SetupAttachment(RootComponent);
	LaserBeamInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamInstances->SetCastShadow(false);

	HazardBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardBox"));
	HazardBox->SetupAttachment(RootComponent);
	HazardBox->SetCollisionProfileName(TEXT("Trigger"));
	HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	HazardBox->SetGenerateOverlapEvents(true);
}

// Initializes laser power state, collision overlaps, and dynamic material
void AGridLaser::BeginPlay()
{
	Super::BeginPlay();

	if (HazardBox)
	{
		HazardBox->OnComponentBeginOverlap.AddDynamic(this, &AGridLaser::OnHazardBeginOverlap);
		HazardBox->OnComponentEndOverlap.AddDynamic(this, &AGridLaser::OnHazardEndOverlap);
	}

	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}

// Scales laser beam and collision box according to length and width in editor
void AGridLaser::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}

#if WITH_EDITOR
// Refreshes laser visuals when length or toggle properties change
void AGridLaser::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}
#endif

// RebuildLaserGrid: controls laser trace and hit detection
void AGridLaser::RebuildLaserGrid()
{
	if (!LaserBeamInstances)
	{
		return;
	}

	if (!LaserBeamInstances->GetStaticMesh())
	{
		if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			LaserBeamInstances->SetStaticMesh(CubeMesh);
		}
	}

	LaserBeamInstances->ClearInstances();

	const float ClampedWidth = FMath::Max(20.0f, GridWidth);
	const float ClampedHeight = FMath::Max(20.0f, GridHeight);
	const float ClampedThickness = FMath::Max(1.0f, BeamThickness);

	if (bEnableHorizontalBeams && NumHorizontalBeams > 0)
	{
		const float StepZ = ClampedHeight / (NumHorizontalBeams + 1.0f);
		const FVector Scale(ClampedThickness / 100.0f, ClampedWidth / 100.0f, ClampedThickness / 100.0f);
		for (int32 i = 0; i < NumHorizontalBeams; ++i)
		{
			LaserBeamInstances->AddInstance(FTransform(FQuat::Identity, FVector(0.0f, 0.0f, StepZ * (i + 1.0f)), Scale));
		}
	}

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

	if (HazardBox)
	{
		const float BoxThickness = FMath::Max(20.0f, ClampedThickness * 2.0f);
		HazardBox->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedHeight * 0.5f));
		HazardBox->SetBoxExtent(FVector(BoxThickness * 0.5f, ClampedWidth * 0.5f, ClampedHeight * 0.5f));
	}
}

// Adjusts beam mesh scale and particle endpoints
void AGridLaser::UpdateLaserVisuals()
{
}

// ConfigureHazardCollision: handles configure hazard collision
void AGridLaser::ConfigureHazardCollision()
{
	if (!HazardBox)
	{
		return;
	}

	if (!bIsLaserOn)
	{
		HazardBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
		return;
	}

	if (bBlockPlayer)
	{
		HazardBox->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	else
	{
		HazardBox->SetCollisionProfileName(TEXT("Trigger"));
		HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
		HazardBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

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

// UpdateLaserState: controls laser trace and hit detection
void AGridLaser::UpdateLaserState()
{
	if (LaserBeamInstances)
	{
		LaserBeamInstances->SetVisibility(bIsLaserOn);
	}
	ConfigureHazardCollision();
}

// SetLaserState: controls laser trace and hit detection
void AGridLaser::SetLaserState(bool bNewActive)
{
	if (bIsLaserOn != bNewActive)
	{
		bIsLaserOn = bNewActive;
		UpdateLaserState();
		OnLaserStateChanged.Broadcast(this, bIsLaserOn);
	}
}

// Inverts current laser active state
void AGridLaser::ToggleLaser()
{
	SetLaserState(!bIsLaserOn);
}

// DealLaserDamage: updates health and processes damage
void AGridLaser::DealLaserDamage(AActor* TargetActor)
{
	if (!TargetActor || !bIsLaserOn)
	{
		return;
	}

	if (AGAME3023_F26_RoshanCharacter* PlayerChar = LiveWireUtils::ResolveActorOrParent<AGAME3023_F26_RoshanCharacter>(TargetActor))
	{
		if (!PlayerChar->IsDead())
		{
			PlayerChar->ApplyDamageToPlayer(DamageAmount, this);
		}
		return;
	}

	if (AEnemyBatteryThief* Thief = LiveWireUtils::ResolveActorOrParent<AEnemyBatteryThief>(TargetActor))
	{
		if (!Thief->bIsDead)
		{
			Thief->KillPermanently();
		}
		return;
	}

	UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, GetInstigatorController(), this, nullptr);
}

// OnHazardBeginOverlap: trigger zone overlap event
void AGridLaser::OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsLaserOn || !OtherActor || OtherActor == this)
	{
		return;
	}

	if (AGAME3023_F26_RoshanCharacter* PlayerChar = LiveWireUtils::ResolveActorOrParent<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		OverlappingPlayer = PlayerChar;
		OverlapDamageTimer = 0.0f;
		DealLaserDamage(PlayerChar);
		return;
	}

	if (AEnemyBatteryThief* Thief = LiveWireUtils::ResolveActorOrParent<AEnemyBatteryThief>(OtherActor))
	{
		DealLaserDamage(Thief);
	}
}

// OnHazardEndOverlap: trigger zone overlap event
void AGridLaser::OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
	}
}

// Updates beam flicker effects or patrol movement if active
void AGridLaser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsLaserOn || !OverlappingPlayer)
	{
		return;
	}

	OverlapDamageTimer += DeltaTime;
	if (OverlapDamageTimer >= DamageTickInterval)
	{
		OverlapDamageTimer = 0.0f;
		DealLaserDamage(OverlappingPlayer);
	}
}
