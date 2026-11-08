// roshan - 101473963 - game3023 game engines 3
// enemy drone header - autonomous patrolling drone with empty static mesh slot,
// downward laser-like spotlight, full-coverage NavMesh patrolling, light-cone player detection & pursuit,
// and periodic laser/bomb particle drops that damage the player while in range

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyDrone.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USpotLightComponent;
class APawn;
class ADroneLaserParticle;

UCLASS()
class GAME3023_F26_ROSHAN_API AEnemyDrone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemyDrone();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// Components
	// ==========================================

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Empty static mesh component - attach or assign custom drone mesh in the editor/Blueprint
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Drone Static Mesh"))
	TObjectPtr<UStaticMeshComponent> DroneMesh;

	// Downward-pointing laser-like spotlight
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Downward Laser Light"))
	TObjectPtr<USpotLightComponent> DownwardLaserLight;

	// ==========================================
	// Instance Editable Patrol & Movement Settings
	// ==========================================

	// Height above the NavMesh floor at which the drone hovers and patrols (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Patrol Height Above NavMesh", ClampMin = "0.0"))
	float PatrolHeight = 350.0f;

	// Movement speed of the drone while patrolling (in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Movement Speed", ClampMin = "10.0"))
	float MovementSpeed = 450.0f;

	// Rotation interpolation speed so the drone smoothly faces its travel direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Rotation Turn Speed", ClampMin = "0.5"))
	float TurnSpeed = 6.0f;

	// Search radius on the NavMesh to sample and cover every reachable area (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "NavMesh Patrol Radius", ClampMin = "100.0"))
	float PatrolRadius = 5000.0f;

	// Grid spacing used to generate waypoints across the entire NavMesh area for full coverage (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Coverage Grid Cell Size", ClampMin = "150.0"))
	float CoverageGridSpacing = 600.0f;

	// Optional manual waypoints. If left empty, the drone automatically generates and patrols all reachable NavMesh points in range.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Custom Patrol Waypoints (Optional)"))
	TArray<TObjectPtr<AActor>> CustomPatrolWaypoints;

	// Brief pause duration when reaching a patrol point before moving to the next (0 = continuous movement)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Movement & Patrol", meta = (DisplayName = "Waypoint Wait Time (Seconds)", ClampMin = "0.0"))
	float WaypointWaitTime = 0.15f;

	// ==========================================
	// Instance Editable Downward Laser Light Settings
	// ==========================================

	// Intensity of the downward laser-like light (instance editable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Intensity", ClampMin = "0.0"))
	float LaserLightIntensity = 50000.0f;

	// Color of the downward laser light
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Color"))
	FLinearColor LaserLightColor = FLinearColor(1.0f, 0.08f, 0.08f);

	// Inner cone angle of the spotlight (kept tight for a focused laser-like beam)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Inner Cone Angle", ClampMin = "0.1", ClampMax = "60.0"))
	float LaserInnerConeAngle = 12.0f;

	// Outer cone angle of the spotlight (determines the light spot on the floor and player detection cone)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Outer Cone Angle", ClampMin = "0.5", ClampMax = "80.0"))
	float LaserOuterConeAngle = 18.0f;

	// Maximum range/attenuation radius of the downward laser light
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Light", meta = (DisplayName = "Laser Light Attenuation Radius", ClampMin = "100.0"))
	float LaserAttenuationRadius = 1500.0f;

	// ==========================================
	// Instance Editable Laser Bomb / Particle Attack Settings
	// ==========================================

	// Amount of damage each dropped laser/bomb particle inflicts on the player's health (instance editable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Damage", ClampMin = "0.0"))
	float LaserParticleDamage = 10.0f;

	// Time interval in seconds between dropping one laser particle and the next while the player is detected in the light (instance editable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Drop Interval (Seconds)", ClampMin = "0.05"))
	float LaserParticleInterval = 0.45f;

	// Downward travel speed of the dropped laser/bomb particles (in cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Fall Speed", ClampMin = "100.0"))
	float LaserParticleSpeed = 1400.0f;

	// Optional custom class for the dropped laser particle (defaults to ADroneLaserParticle)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Laser Bomb Attack", meta = (DisplayName = "Laser Particle Class"))
	TSubclassOf<ADroneLaserParticle> LaserParticleClass;

	// ==========================================
	// Player Detection Under Light
	// ==========================================

	// Extra horizontal padding (in cm) added to the light cone radius on the ground to account for the player capsule width
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drone|Player Detection", meta = (DisplayName = "Detection Capsule Padding", ClampMin = "0.0"))
	float DetectionRadiusPadding = 55.0f;

	// True only while the player is inside the downward light beam and being followed
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Drone|Player Detection")
	bool bIsFollowingPlayer = false;

	// Applies current light settings to the downward laser spotlight component
	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Light")
	void RefreshDroneSettings();

	// Rebuilds the list of NavMesh patrol points covering the area
	UFUNCTION(BlueprintCallable, Category = "Drone|Movement & Patrol")
	void BuildNavMeshCoveragePoints();

	// Returns true if the given pawn is currently illuminated under the downward laser light cone
	UFUNCTION(BlueprintPure, Category = "Drone|Player Detection")
	bool IsPawnUnderLight(const APawn* TargetPawn) const;

	// Spawns a single laser/bomb particle dropping from the drone toward the detected player
	UFUNCTION(BlueprintCallable, Category = "Drone|Laser Bomb Attack")
	void DropLaserParticle(APawn* TargetPawn);

private:
	// Generated or custom 3D ground points on the NavMesh to visit
	UPROPERTY(Transient)
	TArray<FVector> PatrolGroundPoints;

	// Current path corridor points from NavMesh pathfinding to the active destination
	UPROPERTY(Transient)
	TArray<FVector> CurrentPathPoints;

	// Index in CurrentPathPoints we are currently steering toward
	int32 CurrentPathCornerIndex = 0;

	// Index of the active destination in PatrolGroundPoints
	int32 CurrentPatrolIndex = -1;

	// Origin on the NavMesh when patrolling begins
	FVector PatrolOrigin = FVector::ZeroVector;

	// Timer tracking pause at waypoint
	float WaitTimer = 0.0f;

	// Cooldown timer between dropping consecutive laser particles while the player is under the light
	float ParticleDropCooldown = 0.0f;

	// Tracks current smoothed ground Z on the NavMesh underneath the drone
	float CurrentGroundZ = 0.0f;
	bool bHasValidGroundZ = false;

	// Selects the next unvisited NavMesh area point and computes a valid NavMesh path to it
	void AdvanceToNextPatrolDestination();

	// Samples the NavMesh or floor beneath a world XY position to get the ground Z
	float SampleNavMeshGroundZ(const FVector& WorldPos) const;
};
