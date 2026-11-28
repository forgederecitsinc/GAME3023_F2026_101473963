#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CombatDamageable.h"
#include "Engine/TimerHandle.h"
#include "EnemyBatteryThief.generated.h"

class USceneComponent;
class USpotLightComponent;
class USphereComponent;
class UFloatingPawnMovement;
class ARedPowerCylinder;
class ABatteryCapsule;

// States used by the thief AI while it patrols and steals batteries.
UENUM(BlueprintType)
enum class EBatteryThiefState : uint8
{
 // UMETA: umeta
	Patrolling		UMETA(DisplayName = "Patrolling NavMesh"),
 // UMETA: umeta
	Approaching		UMETA(DisplayName = "Moving To Working Battery"),
 // UMETA: umeta
	Removing		UMETA(DisplayName = "Removing Battery (10s Timer)"),
 // UMETA: umeta
	Carrying		UMETA(DisplayName = "Carrying Stolen Battery To Closest BP_BatteryDropSpot"),
 // UMETA: umeta
	FollowingPlayer	UMETA(DisplayName = "Following Player (Speed +70)")
};

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyBatteryThief : public APawn, public ICombatDamageable
{
	GENERATED_BODY()

public:
	// Constructor: creates the thief pawn and its movement and sensor components.
	AEnemyBatteryThief();

protected:
	// Runs when the thief starts in the level.
	virtual void BeginPlay() override;
	// Runs when the thief is removed from the level.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// Updates thief visuals and settings in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// Refreshes settings after an editor property changes.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Runs every frame to update the thief's AI state.
	virtual void Tick(float DeltaTime) override;

	// Bind point for any input components used by the thief.
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

 // Health and damage management
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

 // Health and damage management
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
 // Death and elimination handling
	virtual void HandleDeath() override;
 // ApplyHealing: apply healing
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
 // NotifyDanger: notify danger
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Carried Battery Mount"))
	TObjectPtr<USceneComponent> CarriedBatterySocket;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Scanner Light"))
	TObjectPtr<USpotLightComponent> ScannerLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Pawn Movement"))
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Player Touch Trigger"))
	TObjectPtr<USphereComponent> PlayerTouchTrigger;

	float FollowPlayerSpeedBonus = 70.0f;

	bool bIsFollowingPlayer = false;

	UPROPERTY()
	TObjectPtr<APawn> TargetPlayerPawn = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Player Follow")
 // StartFollowingPlayer: start following player
	void StartFollowingPlayer(APawn* InPlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Player Follow")
 // Navigation and pathfinding
	void StopFollowingPlayerAndResumePatrol();

	UFUNCTION(BlueprintPure, Category = "Battery Thief|Player Follow")
 // IsFollowingPlayer: is following player
	bool IsFollowingPlayer() const { return bIsFollowingPlayer; }

	float MaxHealth = 50.0f;

	float CurrentHealth = 50.0f;

	bool bIsDead = false;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Health & Death")
 // Battery state and handling
	void KillBatteryThief();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Health & Death")
 // KillPermanently: kill permanently
	void KillPermanently();

	FVector SpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	FRotator SpawnRotation = FRotator::ZeroRotator;

	bool bSnapToSpawnLocationOnStart = false;

	float BatteryDetectionRadius = 10000.0f;

	float RemovalInteractionDistance = 230.0f;

	float BatteryRemovalDuration = 10.0f;

	UPROPERTY()
	TSubclassOf<AActor> BatteryDropSpotClass;

	float DropOffAcceptanceRadius = 75.0f;

	EBatteryThiefState CurrentState = EBatteryThiefState::Patrolling;

	UPROPERTY()
	TObjectPtr<ARedPowerCylinder> TargetLaserCylinder = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> TargetDropSpotActor = nullptr;

	float RemovalTimerRemaining = 0.0f;

	UPROPERTY()
	TObjectPtr<ABatteryCapsule> StolenBattery = nullptr;

	float PatrolSpeed = 300.0f;

	float CarrySpeed = 300.0f;

	float TurnSpeed = 8.0f;

	float GroundHeightOffset = 0.0f;

	float PatrolRadius = 4500.0f;

	float AcceptanceRadius = 35.0f;

	float PatrolPointWaitTime = 0.6f;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	FLinearColor ScannerLightColor = FLinearColor(1.0f, 0.35f, 0.05f);

	FLinearColor AlertLightColor = FLinearColor(1.0f, 0.05f, 0.05f);

	float ScannerLightIntensity = 12000.0f;

	float ScannerLightRange = 800.0f;

	float ScannerOuterConeAngle = 28.0f;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief")
 // RefreshEnemySettings: refresh enemy settings
	void RefreshEnemySettings();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Patrol & Movement")
 // Navigation and pathfinding
	void PickNextNavMeshPatrolDestination();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
 // Battery state and handling
	ARedPowerCylinder* FindNearestWorkingBatteryLaser() const;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
 // Battery state and handling
	AActor* FindClosestBatteryDropSpot() const;

	UFUNCTION(BlueprintPure, Category = "Battery Thief|Sabotage & Removal")
 // Battery state and handling
	bool DoesLaserHaveWorkingBattery(const ARedPowerCylinder* Cylinder) const;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
 // Battery state and handling
	void DepositStolenBatteryAtDropOff();

private:
	UPROPERTY(Transient)
	TArray<FVector> CurrentPathPoints;

	int32 CurrentPathCornerIndex = 0;

	int32 CustomWaypointIndex = -1;

	FVector PatrolOrigin = FVector(-878.0f, -3679.0f, 10.0f);

	float InitialSpawnZ = 10.0f;

	float CurrentGroundZ = 10.0f;
	bool bHasValidGroundZ = false;

	FVector CurrentLaserGroundApproachPoint = FVector::ZeroVector;

	float WaitTimer = 0.0f;

	float PathRepathTimer = 0.0f;

 // IsPointOnWalkableSurface: is point on walkable surface
	bool IsPointOnWalkableSurface(const FVector& Point) const;

 // Navigation and pathfinding
	bool IsGroundNavPathValid(const class UNavigationPath* NavPath) const;

 // GetCylinderTriggerGroundCenter: get cylinder trigger ground center
	FVector GetCylinderTriggerGroundCenter(const ARedPowerCylinder* Cylinder) const;

 // ComputeGroundStealApproachPoint: compute ground steal approach point
	bool ComputeGroundStealApproachPoint(const ARedPowerCylinder* Cylinder, FVector& OutGroundPoint, float& OutNavPathLength) const;

 // IsNearestThiefForCylinder: is nearest thief for cylinder
	bool IsNearestThiefForCylinder(const ARedPowerCylinder* Cylinder, float MyGroundPathLength) const;

 // BuildPathToWorldLocation: build path to world location
	bool BuildPathToWorldLocation(const FVector& DestinationWorldPos);

 // Navigation and pathfinding
	void FollowCurrentNavPath(float DeltaTime, float SpeedToUse);

 // Battery state and handling
	void CompleteBatteryRemoval();

 // Navigation and pathfinding
	void ReturnToPatrol();

 // Navigation and pathfinding
	float SampleGroundNavMeshZ(const FVector& WorldPos) const;

	UFUNCTION()
 // Collision overlap callback
	void OnPlayerTouchBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

 // PinToGroundSurface: pin to ground surface
	void PinToGroundSurface();
 // CheckPlayerProximityTouch: check player proximity touch
	void CheckPlayerProximityTouch();
 // Called every frame
	void TickPatrolling(float DeltaTime);
 // Called every frame
	void TickApproaching(float DeltaTime);
 // Called every frame
	void TickRemoving(float DeltaTime);
 // Called every frame
	void TickCarrying(float DeltaTime);
 // Called every frame
	void TickFollowingPlayer(float DeltaTime);
};
