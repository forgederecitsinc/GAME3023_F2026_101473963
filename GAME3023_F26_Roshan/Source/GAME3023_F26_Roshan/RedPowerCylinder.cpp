// roshan - 101473963 - game3023 game engines 3
// red power cylinder implementation
// cell that takes a battery capsule and turns on if hex codes match

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

	// cylinder 3d model
	CylinderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CylinderMesh"));
	CylinderMesh->SetupAttachment(RootComponent);
	CylinderMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CylinderMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 1.2f));

	// grab basic cylinder from engine content
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (MeshFinder.Succeeded())
	{
		CylinderMesh->SetStaticMesh(MeshFinder.Object);
	}

	// base material to tint
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMatFinder(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (BaseMatFinder.Succeeded())
	{
		CylinderMesh->SetMaterial(0, BaseMatFinder.Object);
	}

	// socket on top of cylinder - z=65 puts the battery right on the top surface
	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

	// proximity sphere for pressing E
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(180.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// apply hex color live in the unreal editor viewport
	ApplyColorFromHex();
}

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	// hook up overlap events
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	// initial visual setup
	ApplyColorFromHex();
}

FString ARedPowerCylinder::GetNormalizedHexCode() const
{
	// trim spaces and strip # so "FF0000" and "#ff0000" both work
	FString Clean = ColorHexCode.TrimStartAndEnd();
	if (Clean.StartsWith(TEXT("#")))
	{
		Clean.RightChopInline(1);
	}
	return Clean.ToUpper();
}

void ARedPowerCylinder::ApplyColorFromHex()
{
	if (!CylinderMesh) return;

	FString CleanHex = GetNormalizedHexCode();
	if (CleanHex.Len() < 6)
	{
		CleanHex = TEXT("FF0000"); // default to red
	}
	if (CleanHex.Len() == 6)
	{
		CleanHex += TEXT("FF");
	}

	// convert hex string into unreal color
	const FColor ParsedColor = FColor::FromHex(CleanHex);
	const FLinearColor LinearColor(ParsedColor);

	// create dynamic material instance if needed
	if (!DynamicMaterial)
	{
		DynamicMaterial = CylinderMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		if (bIsPowered)
		{
			// powered ON: make it bright with emissive multiplier so it glows
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), LinearColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), LinearColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), LinearColor * 3.0f);
		}
		else
		{
			// powered OFF: make it dark/dim version of color, no glow
			const FLinearColor DimColor = LinearColor * 0.35f;
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DimColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), DimColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), FLinearColor::Black);
		}
	}
}

void ARedPowerCylinder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARedPowerCylinder::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// show helpful debug messages depending on player state
	if (AGAME3023_F26_RoshanCharacter* Player = Cast<AGAME3023_F26_RoshanCharacter>(OtherActor))
	{
		Player->SetCurrentInteractable(this);

		if (GEngine)
		{
			if (bHasBattery)
			{
				if (Player->HasBattery())
				{
					// player hands are full, tell them to drop first with R
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Silver,
						TEXT("[Live Wire] Cylinder has a Battery. (Drop your battery with [R] to remove it)")
					);
				}
				else
				{
					// empty hands, can remove battery
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Yellow,
						TEXT("[Live Wire] Press [E] to Remove Battery from Cylinder")
					);
				}
			}
			else
			{
				if (Player->HasBattery())
				{
					// cylinder is empty and player has battery, can insert
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Yellow,
						TEXT("[Live Wire] Press [E] to Insert Battery & Power On Cell")
					);
				}
				else
				{
					// both empty
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Orange,
						TEXT("[Live Wire] Red Power Cylinder: OFFLINE (Requires Battery)")
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

	// situation 1: cylinder already has a battery plugged in -> player wants to take it out
	if (bHasBattery)
	{
		if (Player->HasBattery())
		{
			// player is already holding a battery - inventory limit is 1!
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					3.5f,
					FColor::Red,
					TEXT("[INVENTORY FULL] Cannot remove battery! Your hands are already full. Press [R] to drop your current battery first."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}
			return false;
		}

		// player has empty hands, unplug the battery and give it to player
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
					TEXT("[BATTERY REMOVED] Battery removed from Cylinder! Red Power Cylinder is now OFFLINE."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}

			UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] RedPowerCylinder %s: Battery removed by player. Powered OFF."), *GetName());
			return true;
		}
		return false;
	}

	// situation 2: cylinder is empty -> player wants to plug their battery into it
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		return InsertBattery(BatteryToSocket);
	}
	else
	{
		// player pressed E with nothing in hands on an empty cylinder
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.5f,
				FColor::Red,
				TEXT("[POWER OFFLINE] This cell requires a Battery Capsule to power on! Find and collect a battery first."),
				true,
				FVector2D(1.3f, 1.3f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] RedPowerCylinder %s: Interacted without a battery."), *GetName());
		return false;
	}
}

ABatteryCapsule* ARedPowerCylinder::RemoveBattery()
{
	if (!bHasBattery || !InsertedBattery) return nullptr;

	ABatteryCapsule* Extracted = InsertedBattery;

	// reset cylinder power state
	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;

	// turn off glow
	UpdateVisualState();

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;

	// snap battery onto top socket
	Battery->OnSocketed(this, BatterySocket);

	const FString CylinderHex = GetNormalizedHexCode();
	const FString BatteryHex = Battery->GetNormalizedHexCode();

	// compare hex codes: only matching colors turn the power cell ON
	if (CylinderHex.Equals(BatteryHex, ESearchCase::IgnoreCase))
	{
		bIsPowered = true;
		UpdateVisualState(); // turns on bright emissive glow

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				4.5f,
				FColor::Green,
				FString::Printf(TEXT("[HEX MATCH: #%s] Battery attached & Cylinder POWERED ON!"), *CylinderHex),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] RedPowerCylinder %s: Hex MATCH (#%s == #%s). Powered ON!"), *GetName(), *CylinderHex, *BatteryHex);
		return true;
	}
	else
	{
		// wrong color: battery still snaps in, but cylinder stays dead/unpowered
		bIsPowered = false;
		UpdateVisualState();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				5.0f,
				FColor::Orange,
				FString::Printf(TEXT("[HEX MISMATCH] Battery (#%s) attached, but Cylinder (#%s) WON'T POWER ON!"), *BatteryHex, *CylinderHex),
				true,
				FVector2D(1.4f, 1.4f)
			);
		}

		UE_LOG(LogGAME3023_F26_Roshan, Warning, TEXT("[Live Wire] RedPowerCylinder %s: Hex MISMATCH (Battery #%s != Cylinder #%s). Attached but NOT powered!"), *GetName(), *BatteryHex, *CylinderHex);
		return false;
	}
}

void ARedPowerCylinder::UpdateVisualState()
{
	if (!CylinderMesh) return;

	// check if custom materials are assigned in blueprint, otherwise use hex color
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

	ApplyColorFromHex();
}
