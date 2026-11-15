#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDrone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class APawn;
class ADroneLaserParticle;

UENUM(BlueprintType)
enum class EEnemyDroneState : uint8
{
	Patrolling		UMETA(DisplayName = "Patrolling At Z >= 1000"),
	TrackingPlayer	UMETA(DisplayName = "Player Caught (5s Escape Timer)"),
	DivingToLatch	UMETA(DisplayName = "Diving Down To Latch & Kill"),
	LatchedKilled	UMETA(DisplayName = "Latched Onto Player (Killed)")
};

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyDrone : public AActor
{
	GENERATED_BODY()
	
public:	
	AEnemyDrone();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Drone Static Mesh"))
	TObjectPtr<UStaticMeshComponent> DroneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Downward Laser Light"))
	TObjectPtr<USpotLightComponent> DownwardLaserLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Minimum World Z Height", ClampMin = "1000.0"))
	float MinimumFlightZ = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Patrol Height Above NavMesh", ClampMin = "1000.0"))
	float PatrolHeight = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Movement Speed", ClampMin = "10.0"))
	float MovementSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Dive To Latch Speed", ClampMin = "200.0"))
	float DiveLatchSpeed = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Rotation Turn Speed", ClampMin = "0.5"))
	float TurnSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "NavMesh Patrol Radius", ClampMin = "100.0"))
	float PatrolRadius = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Coverage Grid Cell Size", ClampMin = "150.0"))
	float CoverageGridSpacing = 600.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Custom Patrol Waypoints (Optional)"))
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Waypoint Wait Time (Seconds)", ClampMin = "0.0"))
	float WaypointWaitTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Intensity", ClampMin = "0.0"))
	float LaserLightIntensity = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Color"))
	FLinearColor LaserLightColor = FLinearColor(1.0f, 0.08f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Inner Cone Angle", ClampMin = "0.1", ClampMax = "60.0"))
	float LaserInnerConeAngle = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Outer Cone Angle", ClampMin = "0.5", ClampMax = "80.0"))
	float LaserOuterConeAngle = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Attenuation Radius", ClampMin = "100.0"))
	float LaserAttenuationRadius = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Damage", ClampMin = "0.0"))
	float LaserParticleDamage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Drop Interval (Seconds)", ClampMin = "0.05"))
	float LaserParticleInterval = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Fall Speed", ClampMin = "100.0"))
	float LaserParticleSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Class"))
	TSubclassOf<ADroneLaserParticle> LaserParticleClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Player Detection & Latch Kill", meta = (DisplayName = "Escape Countdown Duration (Seconds)", ClampMin = "0.5"))
	float EscapeTimeLimit = 5.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Player Detection & Latch Kill", meta = (DisplayName = "Escape Time Remaining"))
	float EscapeTimerRemaining = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Player Detection & Latch Kill", meta = (DisplayName = "Current Drone State"))
	EEnemyDroneState DroneState = EEnemyDroneState::Patrolling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Player Detection & Latch Kill", meta = (DisplayName = "Detection Capsule Padding", ClampMin = "0.0"))
	float DetectionRadiusPadding = 55.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Player Detection & Latch Kill")
	bool bIsFollowingPlayer = false;

	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Light")
	void RefreshDroneSettings();

	UFUNCTION(BlueprintCallable, Category = "Drone|Movement & Patrol")
	void BuildNavMeshCoveragePoints();

	UFUNCTION(BlueprintCallable, Category = "Drone|Movement & Patrol")
	void ResetToStartAndResumePatrol();

	UFUNCTION(BlueprintPure, Category = "Drone|Player Detection & Latch Kill")
	bool IsPawnUnderLight(const APawn* TargetPawn) const;

	UFUNCTION(BlueprintPure, Category = "Drone|Player Detection & Latch Kill")
	bool IsPawnHidingUnderCover(const APawn* TargetPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Bomb Attack")
	void DropLaserParticle(APawn* TargetPawn);

private:
	UPROPERTY(Transient)
	TArray<FVector> PatrolGroundPoints;

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

	float ComputeHoverAltitudeZ() const;

	void LatchAndKillPlayer(APawn* TargetPawn);

	void AdvanceToNextPatrolDestination();

	float SampleNavMeshGroundZ(const FVector& WorldPos) const;

	bool ClampLocationToNavMesh(const FVector& WorldPos, FVector& OutNavGroundPos, float HorizontalSearchRadius = 500.0f) const;

	void UpdateLightRangeForAltitude(float CurrentZ, float GroundZ);

	void TickLatched(APawn* PlayerPawn, bool bIsPlayerDead);
	void TickDiving(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead);
	void TickTracking(float DeltaTime, APawn* PlayerPawn, bool bIsPlayerDead);
	void TickPatrolling(float DeltaTime, APawn* PlayerPawn);
	void UpdateHoverPosition(const FVector& TargetXY, float FallbackZ, float DeltaTime);
	void FaceDirection2D(const FVector& Dir2D, float DeltaTime, float SpeedMultiplier = 1.0f);
};
