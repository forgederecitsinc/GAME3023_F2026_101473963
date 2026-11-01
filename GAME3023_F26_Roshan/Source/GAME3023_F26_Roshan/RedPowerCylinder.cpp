// Fill out your copyright notice in the Description page of Project Settings.

#include "RedPowerCylinder.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
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

	// Default unpowered material (Gray from project)
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrayMatFinder(TEXT("/Game/_Materials/M_gray.M_gray"));
	if (GrayMatFinder.Succeeded())
	{
		UnpoweredMaterial = GrayMatFinder.Object;
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

void ARedPowerCylinder::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionZone->OnComponentBeginOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionBeginOverlap);
	InteractionZone->OnComponentEndOverlap.AddDynamic(this, &ARedPowerCylinder::OnInteractionEndOverlap);

	UpdateVisualState();
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
			if (bIsPowered)
			{
				GEngine->AddOnScreenDebugMessage(
					2001,
					2.0f,
					FColor::Cyan,
					TEXT("[Live Wire] Red Power Cylinder: ONLINE (100% Power)")
				);
			}
			else if (Player->HasBattery())
			{
				GEngine->AddOnScreenDebugMessage(
					2001,
					2.0f,
					FColor::Yellow,
					TEXT("[Live Wire] Press [E] to Insert Battery & Power On Red Cell")
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

	// Already powered
	if (bIsPowered)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				3.0f,
				FColor::Cyan,
				TEXT("[CELL ONLINE] Red Power Cylinder is already running at 100% power!"),
				true,
				FVector2D(1.2f, 1.2f)
			);
		}
		return false;
	}

	// Player has battery: socket it!
	if (Player->HasBattery())
	{
		ABatteryCapsule* BatteryToSocket = Player->TakeCarriedBattery();
		return InsertBattery(BatteryToSocket);
	}
	else
	{
		// Player has no battery: show error
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

bool ARedPowerCylinder::InsertBattery(ABatteryCapsule* Battery)
{
	if (!Battery) return false;

	InsertedBattery = Battery;
	bHasBattery = true;
	bIsPowered = true;

	// Attach battery to cylinder's socket
	Battery->OnSocketed(this, BatterySocket);

	UpdateVisualState();

	// Success Debug Print
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			4.5f,
			FColor::Green,
			TEXT("[RED CELL POWERED] Battery socketed! Red Power Cylinder is now ONLINE & POWERED!"),
			true,
			FVector2D(1.4f, 1.4f)
		);
	}

	UE_LOG(LogGAME3023_F26_Roshan, Log, TEXT("[Live Wire] RedPowerCylinder %s: Successfully powered ON by %s!"), *GetName(), *Battery->GetName());
	return true;
}

void ARedPowerCylinder::UpdateVisualState()
{
	if (!CylinderMesh) return;

	if (bIsPowered && PoweredMaterial)
	{
		CylinderMesh->SetMaterial(0, PoweredMaterial);
	}
	else if (!bIsPowered && UnpoweredMaterial)
	{
		CylinderMesh->SetMaterial(0, UnpoweredMaterial);
	}
}
