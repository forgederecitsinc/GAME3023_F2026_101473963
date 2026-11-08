// roshan - 101473963 - game3023 game engines 3
// ground battery saboteur / battery thief enemy header
// spawns at (X=-878, Y=-3679, Z=10), patrols the ground NavMesh, detects ONLY working batteries
// that have activated a laser (ARedPowerCylinder::bIsPowered == true), navigates to the nearest working
// battery laser device, runs a 10-second removal timer to detach the battery, scans the NavMesh to find
// the closest BP_BatteryDropSpot actor placed in the level, carries the battery there on its back, drops it, and resumes patrol

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Engine/TimerHandle.h"
#include "EnemyBatteryThief.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class UFloatingPawnMovement;
class ARedPowerCylinder;
class ABatteryCapsule;

UENUM(BlueprintType)
enum class EBatteryThiefState : uint8
{
	Patrolling		UMETA(DisplayName = "Patrolling NavMesh"),
	Approaching		UMETA(DisplayName = "Moving To Working Battery"),
	Removing		UMETA(DisplayName = "Removing Battery (10s Timer)"),
	Carrying		UMETA(DisplayName = "Carrying Stolen Battery To Closest BP_BatteryDropSpot")
};

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyBatteryThief : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AEnemyBatteryThief();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ==========================================
	// Components
	// ==========================================

	// Root scene component on the ground
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Main chassis static mesh (assign custom mesh in BP or use default crawler blockout)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Thief Body Mesh"))
	TObjectPtr<UStaticMeshComponent> ThiefMesh;

	// Socket on top of the chassis where a stolen battery is mounted after the 10s removal completes
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Carried Battery Mount"))
	TObjectPtr<USceneComponent> CarriedBatterySocket;

	// Forward-facing scanner light
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Scanner Light"))
	TObjectPtr<USpotLightComponent> ScannerLight;

	// Floating pawn movement component for smooth ground velocity
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Pawn Movement"))
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	// ==========================================
	// Instance Editable Spawner Settings
	// ==========================================

	// World location where the Battery Thief spawns/starts out of the vent hatch (defaults to X=-878, Y=-3679, Z=10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Location"))
	FVector SpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	// Initial facing rotation when emerging from the spawn hatch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Rotation"))
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// If true, this actor snaps to SpawnLocation on BeginPlay (or acts as a spawner at SpawnLocation if MaxSpawnCount > 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Use Target Spawn Location"))
	bool bSnapToSpawnLocationOnStart = false;

	// If true, this actor acts as a spawner that spawns additional Battery Thief units out of SpawnLocation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Enable Periodic Spawning"))
	bool bActAsSpawner = false;

	// Class of Battery Thief to spawn when bActAsSpawner is enabled (defaults to this actor's own class / BP class)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Enemy Class To Spawn"))
	TSubclassOf<AEnemyBatteryThief> EnemyClassToSpawn;

	// Maximum number of enemies to spawn from SpawnLocation (when bActAsSpawner is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Max Enemies To Spawn", ClampMin = "1", ClampMax = "50"))
	int32 MaxEnemiesToSpawn = 3;

	// Time interval in seconds between spawning enemies at SpawnLocation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Interval (Seconds)", ClampMin = "0.5"))
	float SpawnInterval = 6.0f;

	// Delay in seconds before the first enemy spawns
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Initial Spawn Delay (Seconds)", ClampMin = "0.0"))
	float InitialSpawnDelay = 0.5f;

	// ==========================================
	// Instance Editable Working-Battery Detection, 10s Removal & Drop-Spot Settings
	// ==========================================

	// Detection range (in cm) to scan for active/powered laser guns (where the battery works with the laser)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Working Battery Detection Radius", ClampMin = "100.0"))
	float BatteryDetectionRadius = 10000.0f;

	// Distance (in cm) from the powered laser turret at which the enemy stops and starts the 10-second battery removal timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Removal Reach Distance", ClampMin = "50.0"))
	float RemovalInteractionDistance = 230.0f;

	// Duration in seconds required to disconnect and remove the working battery from the laser (defaults to 10.0 seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Removal Duration (Seconds)", ClampMin = "0.5"))
	float BatteryRemovalDuration = 10.0f;

	// Drop spot Actor class to scan for in the level (automatically defaults to /Game/_Blueprints/LevelAssets/BP_BatteryDropSpot)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Drop Spot Class"))
	TSubclassOf<AActor> BatteryDropSpotClass;

	// Distance threshold (in cm) from the chosen BP_BatteryDropSpot actor at which the enemy deposits the stolen battery
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Drop-Off Reach Distance", ClampMin = "20.0"))
	float DropOffAcceptanceRadius = 75.0f;

	// Current AI state of the Battery Thief
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Current Thief State"))
	EBatteryThiefState CurrentState = EBatteryThiefState::Patrolling;

	// Currently targeted laser device that has a working battery attached
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Target Powered Laser"))
	TObjectPtr<ARedPowerCylinder> TargetLaserCylinder = nullptr;

	// Closest BP_BatteryDropSpot actor found on the NavMesh where the stolen battery is being carried
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Target Drop Spot Actor"))
	TObjectPtr<AActor> TargetDropSpotActor = nullptr;

	// Remaining seconds on the 10-second removal countdown while in the Removing state
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Removal Time Remaining"))
	float RemovalTimerRemaining = 0.0f;

	// Battery currently stolen and carried by this enemy after the 10-second removal completes
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Stolen Battery"))
	TObjectPtr<ABatteryCapsule> StolenBattery = nullptr;

	// ==========================================
	// Instance Editable Ground Movement & NavMesh Patrol Settings
	// ==========================================

	// Ground travel speed while patrolling the NavMesh and approaching a working battery (in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Ground Patrol Speed", ClampMin = "10.0"))
	float PatrolSpeed = 260.0f;

	// Carry travel speed while holding a removed battery (in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Carry Movement Speed", ClampMin = "10.0"))
	float CarrySpeed = 160.0f;

	// How quickly the ground robot turns to face its travel direction along the NavMesh path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Turn Interpolation Speed", ClampMin = "0.5"))
	float TurnSpeed = 8.0f;

	// Vertical offset from the NavMesh floor surface (0 keeps it flush on the ground)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Ground Height Offset"))
	float GroundHeightOffset = 0.0f;

	// Radius around the spawn point on the NavMesh to patrol (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "NavMesh Patrol Radius", ClampMin = "100.0"))
	float PatrolRadius = 4500.0f;

	// Distance threshold (in cm) to consider a NavMesh path corner reached
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Waypoint Acceptance Radius", ClampMin = "10.0"))
	float AcceptanceRadius = 35.0f;

	// Brief pause time in seconds upon reaching each patrol destination before choosing the next NavMesh destination
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Patrol Point Wait Time (Seconds)", ClampMin = "0.0"))
	float PatrolPointWaitTime = 0.6f;

	// Optional designer-placed patrol waypoint actors in the level. If empty, picks reachable points across the NavMesh.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Custom Patrol Waypoints (Optional)"))
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	// ==========================================
	// Instance Editable Visual & Scanner Light Settings
	// ==========================================

	// Scale of the default crawler body mesh
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Appearance", meta = (DisplayName = "Body Mesh Scale"))
	FVector BodyMeshScale = FVector(0.65f, 0.50f, 0.35f);

	// Color of the forward scanning light while patrolling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Patrol Scanner Light Color"))
	FLinearColor ScannerLightColor = FLinearColor(1.0f, 0.35f, 0.05f);

	// Color of the scanner light when a working battery is detected or being removed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Target Locked / Removing Light Color"))
	FLinearColor AlertLightColor = FLinearColor(1.0f, 0.05f, 0.05f);

	// Intensity of the forward scanning light
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Light Intensity", ClampMin = "0.0"))
	float ScannerLightIntensity = 12000.0f;

	// Attenuation radius of the forward scanning light (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Light Range", ClampMin = "50.0"))
	float ScannerLightRange = 800.0f;

	// Cone angle of the forward scanning light
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Outer Cone Angle", ClampMin = "1.0", ClampMax = "80.0"))
	float ScannerOuterConeAngle = 28.0f;

	// Updates light and movement component settings when edited
	UFUNCTION(BlueprintCallable, Category = "Battery Thief")
	void RefreshEnemySettings();

	// Chooses the next reachable destination on the NavMesh and computes a ground path to it
	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Patrol & Movement")
	void PickNextNavMeshPatrolDestination();

	// Scans the level for the nearest ARedPowerCylinder that has a working battery and active laser (bIsPowered && bHasBattery)
	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	ARedPowerCylinder* FindNearestWorkingBatteryLaser() const;

	// Scans all BP_BatteryDropSpot actors in the level and returns the one with the shortest NavMesh path from the enemy
	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	AActor* FindClosestBatteryDropSpot() const;

	// Returns true if the given laser cylinder currently has a working battery that activated its laser
	UFUNCTION(BlueprintPure, Category = "Battery Thief|Sabotage & Removal")
	bool DoesLaserHaveWorkingBattery(const ARedPowerCylinder* Cylinder) const;

	// Drops the currently carried battery at TargetDropSpotActor and returns to patrolling
	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	void DepositStolenBatteryAtDropOff();

	// Spawns a new Battery Thief unit at SpawnLocation (when acting as a spawner)
	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Spawning")
	AEnemyBatteryThief* SpawnThiefAtTargetLocation();

	// Marks this unit as a dynamically spawned child so it doesn't recursively spawn more units
	void MarkAsSpawnedUnit(const FVector& InHomeOrigin);

private:
	// Current NavMesh corridor points to the chosen patrol or target destination
	UPROPERTY(Transient)
	TArray<FVector> CurrentPathPoints;

	// Current corner index along CurrentPathPoints
	int32 CurrentPathCornerIndex = 0;

	// Index when cycling through CustomPatrolWaypoints
	int32 CustomWaypointIndex = -1;

	// Home/spawn origin on the NavMesh used as the center of the patrol territory
	FVector PatrolOrigin = FVector(-878.0f, -3679.0f, 10.0f);

	// Smoothed ground Z height on the NavMesh
	float CurrentGroundZ = 10.0f;
	bool bHasValidGroundZ = false;

	// Pause timer at patrol destination
	float WaitTimer = 0.0f;

	// Periodic path refresh timer when approaching a target laser or drop-off spot
	float PathRepathTimer = 0.0f;

	// Spawner state tracking
	bool bIsSpawnedChildUnit = false;
	int32 TotalSpawnedCount = 0;
	FTimerHandle SpawnTimerHandle;

	// Builds a NavMesh ground path from our current location to a target world location
	bool BuildPathToWorldLocation(const FVector& DestinationWorldPos);

	// Advances movement along CurrentPathPoints at the given speed
	void FollowCurrentNavPath(float DeltaTime, float SpeedToUse);

	// Completes the 10-second removal and detaches the working battery from TargetLaserCylinder
	void CompleteBatteryRemoval();

	// Cancels the current target/removal and returns to patrolling the NavMesh
	void ReturnToPatrol();

	// Samples the NavMesh / floor Z directly underneath the given world position
	float SampleGroundNavMeshZ(const FVector& WorldPos) const;
};
