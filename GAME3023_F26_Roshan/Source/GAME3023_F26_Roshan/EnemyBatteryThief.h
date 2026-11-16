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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBatteryThiefKilled, class AEnemyBatteryThief*, KilledThief);

UENUM(BlueprintType)
enum class EBatteryThiefState : uint8
{
	Patrolling		UMETA(DisplayName = "Patrolling NavMesh"),
	Approaching		UMETA(DisplayName = "Moving To Working Battery"),
	Removing		UMETA(DisplayName = "Removing Battery (10s Timer)"),
	Carrying		UMETA(DisplayName = "Carrying Stolen Battery To Closest BP_BatteryDropSpot"),
	FollowingPlayer	UMETA(DisplayName = "Following Player (Speed +70)")
};

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyBatteryThief : public APawn, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AEnemyBatteryThief();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	UPROPERTY(BlueprintAssignable, Category = "Battery Thief|Health & Death")
	FOnBatteryThiefKilled OnBatteryThiefKilled;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Player Follow", meta = (DisplayName = "Follow Player Speed Bonus", ClampMin = "0.0"))
	float FollowPlayerSpeedBonus = 70.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Player Follow", meta = (DisplayName = "Is Following Player"))
	bool bIsFollowingPlayer = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Player Follow", meta = (DisplayName = "Target Player"))
	TObjectPtr<APawn> TargetPlayerPawn = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Player Follow")
	void StartFollowingPlayer(APawn* InPlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Player Follow")
	void StopFollowingPlayerAndResumePatrol();

	UFUNCTION(BlueprintPure, Category = "Battery Thief|Player Follow")
	bool IsFollowingPlayer() const { return bIsFollowingPlayer; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Health & Death", meta = (DisplayName = "Max Health", ClampMin = "1.0"))
	float MaxHealth = 50.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Battery Thief|Health & Death", meta = (DisplayName = "Current Health"))
	float CurrentHealth = 50.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Health & Death")
	bool bIsDead = false;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Health & Death")
	void KillBatteryThief();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Health & Death")
	void KillPermanently();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Location"))
	FVector SpawnLocation = FVector(-878.0f, -3679.0f, 10.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Rotation"))
	FRotator SpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Use Target Spawn Location"))
	bool bSnapToSpawnLocationOnStart = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Enable Periodic Spawning"))
	bool bActAsSpawner = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Enemy Class To Spawn"))
	TSubclassOf<AEnemyBatteryThief> EnemyClassToSpawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Max Active Enemies In NavMesh", ClampMin = "1", ClampMax = "50"))
	int32 MaxEnemiesToSpawn = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Spawn Interval (Seconds)", ClampMin = "0.5"))
	float SpawnInterval = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Replacement Respawn Delay After Death (Seconds)", ClampMin = "1.0"))
	float RespawnDelayAfterDeath = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Spawning", meta = (DisplayName = "Initial Spawn Delay (Seconds)", ClampMin = "0.0"))
	float InitialSpawnDelay = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Working Battery Detection Radius", ClampMin = "100.0"))
	float BatteryDetectionRadius = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Removal Reach Distance", ClampMin = "50.0"))
	float RemovalInteractionDistance = 230.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Removal Duration (Seconds)", ClampMin = "0.5"))
	float BatteryRemovalDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Battery Drop Spot Class"))
	TSubclassOf<AActor> BatteryDropSpotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Drop-Off Reach Distance", ClampMin = "20.0"))
	float DropOffAcceptanceRadius = 75.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Current Thief State"))
	EBatteryThiefState CurrentState = EBatteryThiefState::Patrolling;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Target Powered Laser"))
	TObjectPtr<ARedPowerCylinder> TargetLaserCylinder = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Target Drop Spot Actor"))
	TObjectPtr<AActor> TargetDropSpotActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Removal Time Remaining"))
	float RemovalTimerRemaining = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battery Thief|Sabotage & Removal", meta = (DisplayName = "Stolen Battery"))
	TObjectPtr<ABatteryCapsule> StolenBattery = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Ground Patrol Speed", ClampMin = "10.0"))
	float PatrolSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Carry Movement Speed", ClampMin = "10.0"))
	float CarrySpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Turn Interpolation Speed", ClampMin = "0.5"))
	float TurnSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Ground Height Offset"))
	float GroundHeightOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Max Ground Step Height", ClampMin = "5.0", ClampMax = "80.0"))
	float MaxGroundStepHeight = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Ground Obstacle Collision Radius", ClampMin = "8.0", ClampMax = "60.0"))
	float GroundCollisionRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "NavMesh Patrol Radius", ClampMin = "100.0"))
	float PatrolRadius = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Waypoint Acceptance Radius", ClampMin = "10.0"))
	float AcceptanceRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Patrol Point Wait Time (Seconds)", ClampMin = "0.0"))
	float PatrolPointWaitTime = 0.6f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Battery Thief|Patrol & Movement", meta = (DisplayName = "Custom Patrol Waypoints (Optional)"))
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Patrol Scanner Light Color"))
	FLinearColor ScannerLightColor = FLinearColor(1.0f, 0.35f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Target Locked / Removing Light Color"))
	FLinearColor AlertLightColor = FLinearColor(1.0f, 0.05f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Light Intensity", ClampMin = "0.0"))
	float ScannerLightIntensity = 12000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Light Range", ClampMin = "50.0"))
	float ScannerLightRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery Thief|Scanner Light", meta = (DisplayName = "Scanner Outer Cone Angle", ClampMin = "1.0", ClampMax = "80.0"))
	float ScannerOuterConeAngle = 28.0f;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief")
	void RefreshEnemySettings();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Patrol & Movement")
	void PickNextNavMeshPatrolDestination();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	ARedPowerCylinder* FindNearestWorkingBatteryLaser() const;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	AActor* FindClosestBatteryDropSpot() const;

	UFUNCTION(BlueprintPure, Category = "Battery Thief|Sabotage & Removal")
	bool DoesLaserHaveWorkingBattery(const ARedPowerCylinder* Cylinder) const;

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Sabotage & Removal")
	void DepositStolenBatteryAtDropOff();

	UFUNCTION(BlueprintCallable, Category = "Battery Thief|Spawning")
	AEnemyBatteryThief* SpawnThiefAtTargetLocation();

	void MarkAsSpawnedUnit(const FVector& InHomeOrigin);

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

	FVector LastStuckCheckPosition = FVector::ZeroVector;
	float StuckWatchdogTimer = 0.0f;
	float WallContactTimer = 0.0f;

	bool bIsSpawnedChildUnit = false;
	int32 TotalSpawnedCount = 0;
	FTimerHandle SpawnTimerHandle;

	bool IsPointOnWalkableSurface(const FVector& Point) const;

	bool IsGroundNavPathValid(const class UNavigationPath* NavPath) const;

	FVector GetCylinderTriggerGroundCenter(const ARedPowerCylinder* Cylinder) const;

	bool ComputeGroundStealApproachPoint(const ARedPowerCylinder* Cylinder, FVector& OutGroundPoint, float& OutNavPathLength) const;

	bool IsNearestThiefForCylinder(const ARedPowerCylinder* Cylinder, float MyGroundPathLength) const;

	bool BuildPathToWorldLocation(const FVector& DestinationWorldPos);

	void FollowCurrentNavPath(float DeltaTime, float SpeedToUse);

	void CompleteBatteryRemoval();

	void ReturnToPatrol();

	float SampleGroundNavMeshZ(const FVector& WorldPos) const;

	UFUNCTION()
	void OnPlayerTouchBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool HandleStuckWatchdog(float DeltaTime, float SpeedToUse, const FVector& CurrentLocation);
	void MoveWithWallSlide(const FVector& CurrentLocation, const FVector& MoveDir2D, float StepDist, float TargetZ, bool bHasNextCorner, float DeltaTime);
	void PinToGroundSurface();
	void CheckPlayerProximityTouch();
	void TickPatrolling(float DeltaTime);
	void TickApproaching(float DeltaTime);
	void TickRemoving(float DeltaTime);
	void TickCarrying(float DeltaTime);
	void TickFollowingPlayer(float DeltaTime);
};
