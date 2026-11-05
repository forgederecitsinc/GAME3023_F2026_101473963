// roshan - 101473963 - game3023 game engines 3
// red power cylinder cpp
// checks if inserted battery's 4-digit ID matches the cell's ID
// shoots out a thin glowing laser beam with instance-editable length when powered on

#include "RedPowerCylinder.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "BatteryCapsule.h"
#include "GAME3023_F26_RoshanCharacter.h"
#include "GAME3023_F26_Roshan.h"

ARedPowerCylinder::ARedPowerCylinder()
{
	PrimaryActorTick.bCanEverTick = true;

	// root component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// cylinder mesh
	CylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CylinderMesh"));
	CylinderMesh->SetupAttachment(RootComponent);
	CylinderMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CylinderMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshFinder.Succeeded())
	{
		CylinderMesh->SetStaticMesh(MeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (BaseMatFinder.Succeeded())
	{
		CylinderMesh->SetMaterial(0, BaseMatFinder.Object);
	}

	// top socket where battery sits
	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

	// laser emitter on the front face of the cylinder
	LaserEmitter = CreateDefaultSubobject<USceneComponent>(TEXT("LaserEmitter"));
	LaserEmitter->SetupAttachment(RootComponent);
	LaserEmitter->SetRelativeLocation(FVector(65.0f, 0.0f, 25.0f)); // front face, mid height

	// laser beam mesh - thin cylinder stretched horizontally
	LaserBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeamMesh"));
	LaserBeamMesh->SetupAttachment(LaserEmitter);
	LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // laser shouldn't block character physically
	LaserBeamMesh->SetCastShadow(false); // laser shouldn't cast shadow

	if (MeshFinder.Succeeded())
	{
		LaserBeamMesh->SetStaticMesh(MeshFinder.Object);
	}
	if (BaseMatFinder.Succeeded())
	{
		LaserBeamMesh->SetMaterial(0, BaseMatFinder.Object);
	}
	LaserBeamMesh->SetVisibility(false); // off until powered on

	// interaction trigger
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(180.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// apply color whenever cell ID changes in editor
	ApplyColorFromID();
	UpdateLaserBeam();
}

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	ApplyColorFromID();
	UpdateLaserBeam();
}

FLinearColor ARedPowerCylinder::GetColorForID(int32 InID) const
{
	// same color map as BatteryCapsule so matching IDs have matching colors automatically
	switch (InID)
	{
		case 1001: return FLinearColor(0.95f, 0.1f, 0.1f);  // red
		case 1002: return FLinearColor(0.1f, 0.9f, 0.15f); // green
		case 1003: return FLinearColor(0.1f, 0.45f, 1.0f); // blue
		case 1004: return FLinearColor(1.0f, 0.85f, 0.1f); // yellow
		case 1005: return FLinearColor(1.0f, 0.4f, 0.05f); // orange
		case 1006: return FLinearColor(0.8f, 0.15f, 1.0f); // purple
		case 1007: return FLinearColor(0.1f, 0.9f, 0.9f);  // cyan
		case 1008: return FLinearColor(1.0f, 0.15f, 0.6f); // magenta
		default:
		{
			// consistent hue for any other 4-digit number
			const uint8 HueByte = (uint8)(FMath::Abs(InID * 53) % 256);
			return FLinearColor::MakeFromHSV8(HueByte, 220, 240);
		}
	}
}

void ARedPowerCylinder::ApplyColorFromID()
{
	if (!CylinderMesh) return;

	const FLinearColor ChosenColor = bUseCustomColor ? CustomColor : GetColorForID(CellID);

	if (!DynamicMaterial)
	{
		DynamicMaterial = CylinderMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		if (bIsPowered)
		{
			// powered ON: bright and glowing
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ChosenColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), ChosenColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), ChosenColor * 3.0f);
		}
		else
		{
			// powered OFF: dim / dark version, no glow
			const FLinearColor DimColor = ChosenColor * 0.35f;
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DimColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), DimColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		}
	}
}

void ARedPowerCylinder::UpdateLaserBeam()
{
	if (!LaserBeamMesh || !LaserEmitter) return;

	// only active if powered on, or if previewing in editor
	const bool bShouldBeActive = bIsPowered || (bPreviewLaserInEditor && GetWorld() && !GetWorld()->IsGameWorld());

	if (!bShouldBeActive)
	{
		LaserBeamMesh->SetVisibility(false);
		return;
	}

	LaserBeamMesh->SetVisibility(true);

	const FVector Start = LaserEmitter->GetComponentLocation();
	const FVector Forward = LaserEmitter->GetForwardVector();
	const FVector DesiredEnd = Start + Forward * LaserLength;

	float ActualDistance = LaserLength;

	// trace forward so laser stops at walls instead of clipping into next room
	if (bLaserStopsAtObstacles && GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (InsertedBattery)
		{
			Params.AddIgnoredActor(InsertedBattery);
		}

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, DesiredEnd, ECC_WorldStatic, Params))
		{
			ActualDistance = FMath::Max(20.0f, Hit.Distance);
		}
	}

	// position laser beam: cylinder mesh origin is center, so offset forward by half distance
	// rotate pitch 90 degrees so cylinder aligns with forward vector
	LaserBeamMesh->SetRelativeLocation(FVector(ActualDistance * 0.5f, 0.0f, 0.0f));
	LaserBeamMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	LaserBeamMesh->SetRelativeScale3D(FVector(LaserThickness / 100.0f, LaserThickness / 100.0f, ActualDistance / 100.0f));

	// create bright emissive laser material
	const FLinearColor ChosenColor = bUseCustomColor ? CustomColor : GetColorForID(CellID);

	if (!LaserDynamicMaterial)
	{
		LaserDynamicMaterial = LaserBeamMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (LaserDynamicMaterial)
	{
		LaserDynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), ChosenColor);
		LaserDynamicMaterial->SetVectorParameterValue(TEXT("Color"), ChosenColor);
		LaserDynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), ChosenColor * 8.0f); // laser intense glow
	}
}

void ARedPowerCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// if cell is powered, continuously trace laser so it reacts if anything moves in front
	if (bIsPowered)
	{
		UpdateLaserBeam();
	}
}

void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);

		if (GEngine)
		{
			if (bHasBattery)
			{
				if (Player->HasBattery())
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Silver,
						FString::Printf(TEXT("[Live Wire] Cylinder (ID: %d) has a Battery. (Drop your battery with [R] first)"), CellID)
					);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Yellow,
						FString::Printf(TEXT("[Live Wire] Press [E] to Remove Battery from Cylinder (ID: %d)"), CellID)
					);
				}
			}
			else
			{
				if (Player->HasBattery())
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Yellow,
						FString::Printf(TEXT("[Live Wire] Press [E] to Insert Battery into Cylinder (ID: %d)"), CellID)
					);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Orange,
						FString::Printf(TEXT("[Live Wire] Cylinder (ID: %d): OFFLINE (Requires Matching Battery)"), CellID)
					);
				}
			}
		}
	}
}

void ARedPowerCylinder::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		if (Player->GetCurrentInteractable() == this)
		{
			Player->SetCurrentInteractable(nullptr);
		}
	}
}

bool ARedPowerCylinder::Interact(AGAME3023_F26_RoshanCharacter* Player)
{
	if (!Player) return false;

	// 1. remove battery if already plugged in
	if (bHasBattery)
	{
		if (Player->HasBattery())
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.5f,
					FColor::Red,
					TEXT("[INVENTORY FULL] Hands full! Press [R] to drop your current battery first."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}
			return false;
		}

		ABatteryCapsule* Extracted = RemoveBattery();
		if (Extracted)
		{
			Player->TryPickupBattery(Extracted);

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					4.0f,
					FColor::Yellow,
					FString::Printf(TEXT("[BATTERY REMOVED] Battery removed! Cylinder (ID: %d) is now OFFLINE."), CellID),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}

			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] RedPowerCylinder %s (ID: %d): Battery removed. Powered OFF."), *GetName(), CellID);
			return true;
		}
		return false;
	}

	// 2. insert battery into empty cylinder
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		const bool bMatched = InsertBattery(BatteryToSocket);

		// dualsense controller rumble feedback: big rumble if ID matched and laser fired, light buzz if mismatch
		if (bMatched)
		{
			Player->TriggerControllerRumble(1.0f, 0.35f, true, true);
		}
		else
		{
			Player->TriggerControllerRumble(0.6f, 0.15f, true, false);
		}

		return bMatched;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.5f,
				FColor::Red,
				FString::Printf(TEXT("[POWER OFFLINE] Cylinder (ID: %d) needs a matching Battery!"), CellID),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] RedPowerCylinder %s (ID: %d): Interacted without a battery."), *GetName(), CellID);
		return false;
	}
}

ABatteryCapsule* ARedPowerCylinder::RemoveBattery()
{
	if (!bHasBattery || !InsertedBattery) return nullptr;

	ABatteryCapsule* Extracted = InsertedBattery;

	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;

	UpdateVisualState();
	UpdateLaserBeam(); // turns off laser!

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;

	// always attach battery to socket
	Battery->OnSocketed(this, BatterySocket);

	// 4-DIGIT ID MATCH CHECK:
	// battery id must equal cell id to power on!
	if (Battery->BatteryID == CellID)
	{
		bIsPowered = true;
		UpdateVisualState(); // turns on glow
		UpdateLaserBeam();   // fires laser!

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.5f,
				FColor::Green,
				FString::Printf(TEXT("[ID MATCH: %d] Battery attached & Cylinder POWERED ON! Laser fired!"), CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] RedPowerCylinder %s: ID MATCH (%d == %d). Powered ON!"), *GetName(), CellID, Battery->BatteryID);
		return true;
	}
	else
	{
		// wrong ID: attaches, but cell stays unpowered
		bIsPowered = false;
		UpdateVisualState();
		UpdateLaserBeam(); // keeps laser off

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Orange,
				FString::Printf(TEXT("[ID MISMATCH] Battery (ID: %d) attached, but Cylinder (ID: %d) WON'T POWER ON!"), Battery->BatteryID, CellID),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] RedPowerCylinder %s: ID MISMATCH (Battery ID: %d != Cell ID: %d). Attached but NOT powered!"), *GetName(), Battery->BatteryID, CellID);
		return false;
	}
}

void ARedPowerCylinder::UpdateVisualState()
{
	if (!CylinderMesh) return;

	if (bIsPowered && PoweredMaterial)
	{
		CylinderMesh->SetMaterial(0, PoweredMaterial);
		return;
	}
	if (!bIsPowered && UnpoweredMaterial)
	{
		CylinderMesh->SetMaterial(0, UnpoweredMaterial);
		return;
	}

	ApplyColorFromID();
}
