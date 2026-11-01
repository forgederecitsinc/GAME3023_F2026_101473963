// Fill out your copyright notice in the Description page of Project Settings.

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

	// 1. Root Component
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 2. Cylinder Mesh
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

	// 3. Battery Socket (Where the battery sits visually on top of the cylinder)
	BatterySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BatterySocket"));
	BatterySocket->SetupAttachment(RootComponent);
	BatterySocket->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));

	// 4. Interaction Proximity Zone
	InteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionZone"));
	InteractionZone->SetupAttachment(RootComponent);
	InteractionZone->SetSphereRadius(180.0f);
	InteractionZone->SetCollisionProfileName(TEXT("Trigger"));
}

void ARedPowerCylinder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColorFromHex();
}

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	ApplyColorFromHex();
}

FString ARedPowerCylinder::GetNormalizedHexCode() const
{
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
		CleanHex = TEXT("FF0000"); // default red
	}
	if (CleanHex.Len() == 6)
	{
		CleanHex += TEXT("FF");
	}

	const FColor ParsedColor = FColor::FromHex(CleanHex);
	const FLinearColor LinearColor(ParsedColor);

	if (!DynamicMaterial)
	{
		DynamicMaterial = CylinderMesh->CreateAndSetMaterialInstanceDynamic(0);
	}

	if (DynamicMaterial)
	{
		if (bIsPowered)
		{
			// Bright, fully powered color + emissive glow
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), LinearColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), LinearColor);
			DynamicMaterial->SetVectorParameterValue(TEXT("Emissive"), LinearColor * 3.0f);
		}
		else
		{
			// Dim, offline unpowered shade of the assigned color
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
						TEXT("[Live Wire] Cylinder has a Battery. (Drop your battery with [R] to remove it)")
					);
				}
				else
				{
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
					GEngine->AddOnScreenDebugMessage(
						2001,
						2.0f,
						FColor::Yellow,
						TEXT("[Live Wire] Press [E] to Insert Battery & Power On Cell")
					);
				}
				else
				{
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

	// Case 1: Cylinder ALREADY HAS a battery -> Player wants to REMOVE it
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
					TEXT("[INVENTORY FULL] Cannot remove battery! Your hands are already full. Press [R] to drop your current battery first."),
					true,
					FVector2D(1.3f, 1.3f)
				);
			}
			return false;
		}

		// Player has empty hands: extract battery and give to player
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

	// Case 2: Cylinder is EMPTY -> Player wants to INSERT a battery
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		return InsertBattery(BatteryToSocket);
	}
	else
	{
		// Empty cylinder and player has no battery
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

	InsertedBattery = nullptr;
	bHasBattery = false;
	bIsPowered = false;

	UpdateVisualState();

	return Extracted;
}

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;

	// Always attach the battery to the cylinder socket
	Battery->OnSocketed(this, BatterySocket);

	const FString CylinderHex = GetNormalizedHexCode();
	const FString BatteryHex = Battery->GetNormalizedHexCode();

	// EXACT HEX MATCH CHECK: Only capsules with the matching hex code power the cylinder ON
	if (CylinderHex.Equals(BatteryHex, ESearchCase::IgnoreCase))
	{
		bIsPowered = true;
		UpdateVisualState();

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
		// Mismatched Hex Code: Attaches, but cylinder DOES NOT power on
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
