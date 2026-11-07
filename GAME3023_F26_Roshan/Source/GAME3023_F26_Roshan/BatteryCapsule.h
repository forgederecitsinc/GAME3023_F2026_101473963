// roshan - 101473963 - game3023 game engines 3
// battery capsule header - redesigned as an industrial power cell battery
// matches the arena blockout style with dark foundation plinth, cream pedestal, colored cylinder core, and dark top terminal cap

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BatteryCapsule.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AGAME3023_F26_RoshanCharacter;

UCLASS()
class GAME3023_F26_ROSHAN_API ABatteryCapsule : public AActor
{
	GENERATED_BODY()
	
public:	
	ABatteryCapsule();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// 4-digit ID for this battery (default 1001)
	// battery id has to match the cylinder cell id to power it on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Battery ID (4-Digit)"))
	int32 BatteryID = 1001;

	// Legacy backward-compatibility aliases (non-instance-editable)
	UPROPERTY()
	bool bUseCustomColor = false;

	UPROPERTY()
	FLinearColor CustomColor = FLinearColor(0.0000f, 0.8714f, 1.0000f);

	// Base mesh foundation colors (fixed industrial palette matching arena blockout)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Base Plinth Color"))
	FLinearColor PlinthColor = FLinearColor(0.0395f, 0.0497f, 0.0723f); // #383F4C

	// Base pedestal slab color (matte deep slate / charcoal #1A1D24)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Base Pedestal Color"))
	FLinearColor PedestalColor = FLinearColor(0.0103f, 0.0123f, 0.0176f); // #1A1D24

	// Top terminal cap color (anodized dark alloy #222730)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Top Cap Color"))
	FLinearColor TopCapColor = FLinearColor(0.0160f, 0.0203f, 0.0296f); // #222730

	// runtime dynamic material instance for battery cylinder glow
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	// updates material color based on ID
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Color")
	void ApplyColorFromID();

	// converts any 4 digit id into a consistent color matching cylinders
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// collision capsule
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// base bottom plinth - sharp dark square foundation slab (matches arena battery dock pad)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePlinthMesh;

	// base pedestal plate - sharp middle tier slab (matches arena battery pedestal)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BasePedestalMesh;

	// central battery cylinder cell mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BatteryMesh;

	// top terminal cap - sharp dark square block on top of the cylinder
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TopCapMesh;

	// interaction trigger zone for picking up battery
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionZone;

	// whether to show the bottom base dock pad and pedestal
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Show Base Dock & Pedestal"))
	bool bShowBaseStructure = true;

	// whether to show the top square terminal cap
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Show Top Terminal Cap"))
	bool bShowTopCap = true;

	// whether to hide the base dock and pedestal when carried on backpack or socketed into turret
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Appearance", meta = (DisplayName = "Hide Base When Picked Up / Socketed"))
	bool bHideBaseWhenCarried = true;

	// optional material overrides
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Base Plinth Material"))
	TObjectPtr<UMaterialInterface> PlinthMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Base Pedestal Material"))
	TObjectPtr<UMaterialInterface> PedestalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Top Cap Material"))
	TObjectPtr<UMaterialInterface> TopCapMaterial;

	// constructs and configures battery subcomponents with exact proportions
	UFUNCTION(BlueprintCallable, Category = "Battery|Visual")
	void BuildBatteryComponents();

	// idle bobbing and spin settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	bool bEnableIdleAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float RotationSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobFrequency = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battery|Visual")
	float BobAmplitude = 10.0f;

	// state flags
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsCarried = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	bool bIsSocketed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Battery|State")
	FVector SpawnOrigin;

	// interaction functions
	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	bool Interact(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnCollected(AGAME3023_F26_RoshanCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnSocketed(AActor* SocketActor, USceneComponent* AttachTargetComp);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void OnDropped(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category = "Battery|Actions")
	void RespawnToOrigin();

protected:
	// overlap events
	UFUNCTION()
	void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	float RunningTime = 0.0f;
	FVector BaseLocation;
};
