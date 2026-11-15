#include "GridLaser.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "EnemyBatteryThief.h"
#include "LiveWireUtils.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"

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

void AGridLaser::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}

#if WITH_EDITOR
void AGridLaser::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLaserGrid();
	UpdateLaserVisuals();
	UpdateLaserState();
}
#endif

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

void AGridLaser::UpdateLaserVisuals()
{
}

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

void AGridLaser::UpdateLaserState()
{
	if (LaserBeamInstances)
	{
		LaserBeamInstances->SetVisibility(bIsLaserOn);
	}
	ConfigureHazardCollision();
}

void AGridLaser::SetLaserState(bool bNewActive)
{
	if (bIsLaserOn != bNewActive)
	{
		bIsLaserOn = bNewActive;
		UpdateLaserState();
		OnLaserStateChanged.Broadcast(this, bIsLaserOn);
	}
}

void AGridLaser::ToggleLaser()
{
	SetLaserState(!bIsLaserOn);
}

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

void AGridLaser::OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
	}
}

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
