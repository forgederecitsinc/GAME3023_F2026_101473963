#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDrone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class APawn;
class ADroneLaserParticle;

// Possible behavior states for an enemy drone.
UENUM(BlueprintType)
enum class EEnemyDroneState : uint8
{
 // UMETA: umeta
	Patrolling		UMETA(DisplayName = "Patrolling At Z >= 1000"),
 // UMETA: umeta
	TrackingPlayer	UMETA(DisplayName = "Player Caught (5s Escape Timer)"),
 // UMETA: umeta
	DivingToLatch	UMETA(DisplayName = "Diving Down To Latch & Kill"),
 // UMETA: umeta
	LatchedKilled	UMETA(DisplayName = "Latched Onto Player (Killed)")
};

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyDrone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Constructor: creates the drone mesh, light, and base components.
	AEnemyDrone();

protected:
	// Runs when the drone first starts in the level.
	virtual void BeginPlay() override;

	// Updates the drone preview while it is placed or changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// Rebuilds the drone settings after an editor property changes.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Runs every frame to update the selected drone behavior state.
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Drone Static Mesh"))
	TObjectPtr<UStaticMeshComponent> DroneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Downward Laser Light"))
	TObjectPtr<USpotLightComponent> DownwardLaserLight;

	float MinimumFlightZ = 1000.0f;

	float PatrolHeight = 1500.0f;

	float MovementSpeed = 450.0f;

	float DiveLatchSpeed = 2200.0f;

	float TurnSpeed = 6.0f;

	float PatrolRadius = 5000.0f;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	float WaypointWaitTime = 0.15f;

	float LaserLightIntensity = 50000.0f;

	FLinearColor LaserLightColor = FLinearColor(1.0f, 0.08f, 0.08f);

	float LaserInnerConeAngle = 12.0f;

	float LaserOuterConeAngle = 18.0f;

	float LaserAttenuationRadius = 3500.0f;

	float LaserParticleDamage = 10.0f;

	float LaserParticleInterval = 0.45f;

	float LaserParticleSpeed = 1400.0f;

	UPROPERTY()
	TSubclassOf<ADroneLaserParticle> LaserParticleClass;

	float EscapeTimeLimit = 5.0f;

	float EscapeTimerRemaining = 0.0f;

	EEnemyDroneState DroneState = EEnemyDroneState::DivingToLatch;

	float DetectionRadiusPadding = 55.0f;

	bool bIsFollowingPlayer = false;

	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Light")
	// Applies the current spotlight and movement settings.
	void RefreshDroneSettings();

	UFUNCTION(BlueprintCallable, Category = "Drone|Movement & Patrol")
	// Returns the drone to its starting position and restarts patrol.
	void ResetToStartAndResumePatrol();

	UFUNCTION(BlueprintPure, Category = "Drone|Player Detection & Latch Kill")
	// Checks whether the player pawn is inside the drone's spotlight.
	bool IsPawnUnderLight(const APawn* TargetPawn) const;

	UFUNCTION(BlueprintPure, Category = "Drone|Player Detection & Latch Kill")
	// Checks whether nearby cover blocks the drone's line of sight.
	bool IsPawnHidingUnderCover(const APawn* TargetPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Bomb Attack")
	// Drops a laser particle toward the target pawn.
	void DropLaserParticle(APawn* TargetPawn);

private:
	UPROPERTY(Transient)
	TArray<FVector> CurrentPathPoints;

	int32 CurrentPathCornerIndex = 0;

	int32 CurrentPatrolIndex = -1;

	FVector PatrolOrigin = FVector::ZeroVector;

	FVector InitialSpawnLocation = FVector::ZeroVector;
	FRotator InitialSpawnRotation = FRotator::ZeroRotator;
	float InitialGroundZ = 0.0f;

	float WaitTimer = 0.0f;

	float ParticleDropCooldown = 0.0f;

	float CurrentGroundZ = 0.0f;
	bool bHasValidGroundZ = false;

	// Calculates the drone's safe hover height above the ground.
	float ComputeHoverAltitudeZ() const;

	// Starts the diving latch sequence and kills the player.
	void LatchAndKillPlayer(APawn* TargetPawn);

	// Chooses the next patrol waypoint.
	void AdvanceToNextPatrolDestination();

	// Samples the ground height under a patrol position.
	float SampleNavMeshGroundZ(const FVector& WorldPos) const;

	// Keeps a world location inside a valid navigation area.
	bool ClampLocationToNavMesh(const FVector& WorldPos, FVector& OutNavGroundPos, float HorizontalSearchRadius = 500.0f) const;

	// Adjusts spotlight distance based on the drone's current height.
	void UpdateLightRangeForAltitude(float CurrentZ, float GroundZ);

	// Handles behavior while latched onto the player.
	void TickLatched(APawn* PlayerPawn, bool bIsPlayerDead);

	// Handles the drone as it dives toward the player.
	void TickDiving(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead);

	// Handles the escape countdown while the player is caught.
	void TickTracking(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead);

	// Handles normal patrol movement.
	void TickPatrolling(float DeltaTime, APawn* PlayerPawn);

	// Smoothly moves the drone toward a target hover position.
	void UpdateHoverPosition(const FVector& TargetXY, float FallbackZ, float DeltaTime);

	// Rotates the drone toward a horizontal direction.
	void FaceDirection2D(const FVector& Dir2D, float DeltaTime, float SpeedMultiplier = 1.0f);
};
