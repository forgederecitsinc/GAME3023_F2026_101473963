#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLaser.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;
class AGridLaser;

// Grid laser hazard that creates a grid of beams and damages the player.
UCLASS()
class GAME3023_F26_ROSHAN_API AGridLaser : public AActor
{
	GENERATED_BODY()

public:
	// Creates the laser components when the actor is spawned.
	AGridLaser();

	// Runs every frame so the laser can update damage and movement blocking.
	virtual void Tick(float DeltaTime) override;

	// Controls whether the laser is currently active.
	bool bIsLaserOn = true;

	// Controls whether this hazard blocks the player's movement.
	bool bBlockPlayer = false;

	// Sets the width of the laser grid.
	float GridWidth = 400.0f;

	// Sets the height of the laser grid.
	float GridHeight = 300.0f;

	// Sets how many beams are placed across the grid horizontally.
	int32 NumHorizontalBeams = 5;

	// Sets how many beams are placed across the grid vertically.
	int32 NumVerticalBeams = 4;

	// Shows or hides the horizontal beams.
	bool bEnableHorizontalBeams = true;

	// Shows or hides the vertical beams.
	bool bEnableVerticalBeams = true;

	// Sets how thick each laser beam appears.
	float BeamThickness = 8.0f;

	// Sets how much damage the laser deals on each damage tick.
	float DamageAmount = 25.0f;

	// Sets the time between each damage tick while the player overlaps the laser.
	float DamageTickInterval = 0.5f;

	// Returns true while the laser is active.
	UFUNCTION(BlueprintPure, Category = "LiveWire|GridLaser")
	bool IsLaserOn() const { return bIsLaserOn; }

	// Rebuilds all of the laser beams using the current grid settings.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void RebuildLaserGrid();

	// Updates the visible and collision state of the laser.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserState();

protected:
	// Runs when the game starts or when the actor is spawned.
	virtual void BeginPlay() override;

	// Runs when the actor is placed or changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// Rebuilds the laser when a property is changed in the editor.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Root component used as the base position for the laser.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Instanced mesh component that draws all laser beams.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> LaserBeamInstances;

	// Box collision used to detect when the player enters the laser area.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HazardBox;

private:
	// Player currently overlapping the laser hazard.
	UPROPERTY(Transient)
	TObjectPtr<AGAME3023_F26_RoshanCharacter> OverlappingPlayer;

	// Tracks how much time has passed before the next damage tick.
	float OverlapDamageTimer = 0.0f;

	// Sets up the size and collision settings for the hazard box.
	void ConfigureHazardCollision();

	// Damages an actor that is currently inside the laser hazard.
	void DealLaserDamage(AActor* TargetActor);

	UFUNCTION()
	// Runs when an actor first enters the laser hazard.
	void OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	// Runs when an actor leaves the laser hazard.
	void OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
