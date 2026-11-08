// roshan - 101473963 - game3023 game engines 3
// grid laser implementation - customizable security laser grid barrier / trap
// supports instance-editable dimensions, beam density, glowing materials, and instant editor on/off toggling

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

// Sets default values
AGridLaser::AGridLaser()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. Root scene component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Load standard engine shapes and project materials
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* LaserMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/_Materials/gameplay/M_laserbeam.M_laserbeam"));
	if (!LaserMat)
	{
		LaserMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	LaserMaterialAsset = LaserMat;

	UMaterialInterface* FrameMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (!FrameMat)
	{
		FrameMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	FrameMaterialAsset = FrameMat;

	// 2. Instanced static mesh component for batched laser beams
	LaserBeamInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LaserBeamInstances"));
	LaserBeamInstances->SetupAttachment(RootComponent);
	LaserBeamInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeamInstances->SetCastShadow(false);
	if (CubeMesh)
	{
		LaserBeamInstances->SetStaticMesh(CubeMesh);
	}
	if (LaserMaterialAsset)
	{
		LaserBeamInstances->SetMaterial(0, LaserMaterialAsset);
	}

	// 3. Hazard overlap / collision box for the laser grid barrier
	HazardBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardBox"));
	HazardBox->SetupAttachment(RootComponent);
	HazardBox->SetCollisionProfileName(TEXT("Trigger"));
	HazardBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	HazardBox->SetGenerateOverlapEvents(true);

	// 4. Emitter frame posts / borders
	LeftPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPillarMesh"));
	LeftPillarMesh->SetupAttachment(RootComponent);
	LeftPillarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) LeftPillarMesh->SetStaticMesh(CubeMesh);
	if (FrameMaterialAsset) LeftPillarMesh->SetMaterial(0, FrameMaterialAsset);

	RightPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPillarMesh"));
	RightPillarMesh->SetupAttachment(RootComponent);
	RightPillarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) RightPillarMesh->SetStaticMesh(CubeMesh);
	if (FrameMaterialAsset) RightPillarMesh->SetMaterial(0, FrameMaterialAsset);

	TopPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TopPillarMesh"));
	TopPillarMesh->SetupAttachment(RootComponent);
	TopPillarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) TopPillarMesh->SetStaticMesh(CubeMesh);
	if (FrameMaterialAsset) TopPillarMesh->SetMaterial(0, FrameMaterialAsset);

	BottomPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BottomPillarMesh"));
	BottomPillarMesh->SetupAttachment(RootComponent);
	BottomPillarMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	if (CubeMesh) BottomPillarMesh->SetStaticMesh(CubeMesh);
	if (FrameMaterialAsset) BottomPillarMesh->SetMaterial(0, FrameMaterialAsset);
}

// Called when the game starts or when spawned
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

// Rebuilds all laser beams and emitter border dimensions
void AGridLaser::RebuildLaserGrid()
{
	if (!LaserBeamInstances)
	{
		return;
	}

	// Make sure mesh is assigned
	if (!LaserBeamInstances->GetStaticMesh())
	{
		UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh)
		{
			LaserBeamInstances->SetStaticMesh(CubeMesh);
		}
	}

	// Clear previous instances
	LaserBeamInstances->ClearInstances();

	const float ClampedWidth = FMath::Max(20.0f, GridWidth);
	const float ClampedHeight = FMath::Max(20.0f, GridHeight);
	const float ClampedThickness = FMath::Max(1.0f, BeamThickness);

	// 1. Horizontal Beams (spanning along Y, spaced along Z)
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

	// 2. Vertical Beams (spanning along Z, spaced along Y)
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

	// 3. Emitter Frame Posts
	const float FrameThickness = FMath::Max(5.0f, PillarThickness);
	const bool bShowFrames = bShowEmitterPillars;

	if (LeftPillarMesh)
	{
		LeftPillarMesh->SetVisibility(bShowFrames);
		LeftPillarMesh->SetCollisionEnabled(bShowFrames ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bShowFrames)
		{
			LeftPillarMesh->SetRelativeLocation(FVector(0.0f, (-ClampedWidth * 0.5f) - (FrameThickness * 0.5f), ClampedHeight * 0.5f));
			LeftPillarMesh->SetRelativeScale3D(FVector(FrameThickness / 100.0f, FrameThickness / 100.0f, (ClampedHeight + FrameThickness * 2.0f) / 100.0f));
			if (FrameMaterialAsset) LeftPillarMesh->SetMaterial(0, FrameMaterialAsset);
		}
	}

	if (RightPillarMesh)
	{
		RightPillarMesh->SetVisibility(bShowFrames);
		RightPillarMesh->SetCollisionEnabled(bShowFrames ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bShowFrames)
		{
			RightPillarMesh->SetRelativeLocation(FVector(0.0f, (ClampedWidth * 0.5f) + (FrameThickness * 0.5f), ClampedHeight * 0.5f));
			RightPillarMesh->SetRelativeScale3D(FVector(FrameThickness / 100.0f, FrameThickness / 100.0f, (ClampedHeight + FrameThickness * 2.0f) / 100.0f));
			if (FrameMaterialAsset) RightPillarMesh->SetMaterial(0, FrameMaterialAsset);
		}
	}

	if (TopPillarMesh)
	{
		TopPillarMesh->SetVisibility(bShowFrames);
		TopPillarMesh->SetCollisionEnabled(bShowFrames ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bShowFrames)
		{
			TopPillarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedHeight + (FrameThickness * 0.5f)));
			TopPillarMesh->SetRelativeScale3D(FVector(FrameThickness / 100.0f, (ClampedWidth + FrameThickness * 2.0f) / 100.0f, FrameThickness / 100.0f));
			if (FrameMaterialAsset) TopPillarMesh->SetMaterial(0, FrameMaterialAsset);
		}
	}

	if (BottomPillarMesh)
	{
		BottomPillarMesh->SetVisibility(bShowFrames);
		BottomPillarMesh->SetCollisionEnabled(bShowFrames ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (bShowFrames)
		{
			BottomPillarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -FrameThickness * 0.5f));
			BottomPillarMesh->SetRelativeScale3D(FVector(FrameThickness / 100.0f, (ClampedWidth + FrameThickness * 2.0f) / 100.0f, FrameThickness / 100.0f));
			if (FrameMaterialAsset) BottomPillarMesh->SetMaterial(0, FrameMaterialAsset);
		}
	}

	// 4. Hazard Trigger Box spanning the grid area
	if (HazardBox)
	{
		const float BoxThickness = FMath::Max(20.0f, ClampedThickness * 2.0f);
		HazardBox->SetRelativeLocation(FVector(0.0f, 0.0f, ClampedHeight * 0.5f));
		HazardBox->SetBoxExtent(FVector(BoxThickness * 0.5f, ClampedWidth * 0.5f, ClampedHeight * 0.5f));
	}
}

// Updates glowing dynamic material parameters
void AGridLaser::UpdateLaserVisuals()
{
	if (!LaserBeamInstances)
	{
		return;
	}

	if (!LaserDynamicMat && LaserMaterialAsset)
	{
		LaserDynamicMat = LaserBeamInstances->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (LaserDynamicMat)
	{
		const FLinearColor EmissiveColor = LaserColor * LaserEmissiveIntensity;
		LaserDynamicMat->SetVectorParameterValue(TEXT("Base Color"), LaserColor);
		LaserDynamicMat->SetVectorParameterValue(TEXT("BaseColor"), LaserColor);
		LaserDynamicMat->SetVectorParameterValue(TEXT("Color"), LaserColor);
		LaserDynamicMat->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
		LaserDynamicMat->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
		LaserDynamicMat->SetVectorParameterValue(TEXT("Emissive"), EmissiveColor);
	}
}

// Updates visibility and collision based on bIsLaserOn (instant editor toggle)
void AGridLaser::UpdateLaserState()
{
	if (LaserBeamInstances)
	{
		LaserBeamInstances->SetVisibility(bIsLaserOn);
	}

	if (HazardBox)
	{
		if (bIsLaserOn)
		{
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

			// If actors are already inside when lasers turn on, deal lethal contact immediately
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
			HazardBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			OverlappingPlayer = nullptr;
			OverlapDamageTimer = 0.0f;
		}
	}
}

// Turns the laser grid on or off at runtime
void AGridLaser::SetLaserState(bool bNewActive)
{
	if (bIsLaserOn != bNewActive)
	{
		bIsLaserOn = bNewActive;
		UpdateLaserState();
		OnLaserStateChanged.Broadcast(this, bIsLaserOn);
	}
}

// Toggles the laser grid state
void AGridLaser::ToggleLaser()
{
	SetLaserState(!bIsLaserOn);
}

// Applies damage to player or destroys Battery Thief on laser contact
void AGridLaser::DealLaserDamage(AActor* TargetActor)
{
	if (!TargetActor || !bIsLaserOn)
	{
		return;
	}

	// 1. Check if the player contacted the laser grid
	AGAME3023_F26_RoshanCharacter* PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetActor);
	if (!PlayerChar)
	{
		PlayerChar = Cast<AGAME3023_F26_RoshanCharacter>(TargetActor->GetAttachParentActor());
	}

	if (PlayerChar && !PlayerChar->IsDead())
	{
		// Player health is damaged by DamageAmount (10 points) on contact/passing through
		PlayerChar->ApplyDamageToPlayer(DamageAmount, this);
		return;
	}

	// 2. Check if the Battery Thief enemy contacted the laser grid
	AEnemyBatteryThief* Thief = Cast<AEnemyBatteryThief>(TargetActor);
	if (!Thief)
	{
		Thief = Cast<AEnemyBatteryThief>(TargetActor->GetAttachParentActor());
	}

	if (Thief && !Thief->bIsDead)
	{
		// Battery Thief dies permanently (safely drops battery and will NOT respawn)
		Thief->KillPermanently();
		return;
	}

	// Fallback for any other damageable actors
	UGameplayStatics::ApplyDamage(TargetActor, DamageAmount, GetInstigatorController(), this, nullptr);
}

void AGridLaser::OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsLaserOn || !OtherActor || OtherActor == this)
	{
		return;
	}

	// 1. Player overlap
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

	// 2. Battery Thief overlap
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

void AGridLaser::OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherActor == OverlappingPlayer)
	{
		OverlappingPlayer = nullptr;
		OverlapDamageTimer = 0.0f;
	}
}

// Called every frame
void AGridLaser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// If player is continuously standing inside active laser grid, deal periodic burn damage
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
