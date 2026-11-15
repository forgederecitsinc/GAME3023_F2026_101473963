// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Security Grid Laser Barrier: renders instanced laser beams and damages/blocks actors.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridLaser.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridLaserStateChanged, AGridLaser*, LaserGrid, bool, bIsActive);

UCLASS()
class GAME3023_F26_ROSHAN_API AGridLaser : public AActor
{
	GENERATED_BODY()

public:
	AGridLaser();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Is Laser On"))
	bool bIsLaserOn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser", meta = (DisplayName = "Block Player Movement"))
	bool bBlockPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridWidth = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Dimensions", meta = (ClampMin = "50.0", UIMin = "50.0"))
	float GridHeight = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumHorizontalBeams = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "0", UIMin = "0"))
	int32 NumVerticalBeams = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableHorizontalBeams = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams")
	bool bEnableVerticalBeams = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Beams", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BeamThickness = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals")
	FLinearColor LaserColor = FLinearColor(1.0f, 0.05f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Visuals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LaserEmissiveIntensity = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DamageAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|GridLaser|Damage", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float DamageTickInterval = 0.5f;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void SetLaserState(bool bNewActive);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void ToggleLaser();

	UFUNCTION(BlueprintPure, Category = "LiveWire|GridLaser")
	bool IsLaserOn() const { return bIsLaserOn; }

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|GridLaser")
	FOnGridLaserStateChanged OnLaserStateChanged;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void RebuildLaserGrid();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserState();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|GridLaser")
	void UpdateLaserVisuals();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> LaserBeamInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HazardBox;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LaserDynamicMat;

	UPROPERTY(Transient)
	TObjectPtr<AGAME3023_F26_RoshanCharacter> OverlappingPlayer;

	float OverlapDamageTimer = 0.0f;

	void ConfigureHazardCollision();
	void DealLaserDamage(AActor* TargetActor);

	UFUNCTION()
	void OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHazardEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
