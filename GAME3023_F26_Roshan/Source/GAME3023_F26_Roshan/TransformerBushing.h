#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransformerBushing.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPointLightComponent;
class USoundBase;
class ALaserTarget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTransformerBushingStateChanged, class ATransformerBushing*, Bushing, bool, bIsLit);

UCLASS()
class GAME3023_F26_ROSHAN_API ATransformerBushing : public AActor
{
	GENERATED_BODY()

public:
	ATransformerBushing();

 // Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Bushing ID (4-Digit)"))
	int32 BushingID = 1001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Target ID (Alias)"))
	int32 TargetID = 1001;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Static Mesh Component"))
	TObjectPtr<UStaticMeshComponent> BushingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Bushing Light"))
	TObjectPtr<UPointLightComponent> BushingLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Use Custom Neon Color"))
	bool bUseCustomNeonColor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Lit Color"))
	FLinearColor NeonLitColor = FLinearColor(0.00f, 0.95f, 1.00f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Unlit Color"))
	FLinearColor NeonUnlitColor = FLinearColor(0.04f, 0.04f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Emissive Multiplier", ClampMin = "1.0", ClampMax = "100.0"))
	float LitEmissiveMultiplier = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Unlit Emissive Multiplier"))
	float UnlitEmissiveMultiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Lit Light Intensity (Lumens)"))
	float LitLightIntensity = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Light Radius"))
	float LightAttenuationRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Enable Neon Ignition Flicker"))
	bool bEnableNeonIgnitionFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Preview Neon Lit in Editor"))
	bool bPreviewLitInEditor = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Lit Up"))
	bool bIsLitUp = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Stay Lit Once Activated"))
	bool bStayLitOnceActivated = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Linked Laser Target (Optional)"))
	TObjectPtr<ALaserTarget> LinkedLaserTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Light Up Sound"))
	TObjectPtr<USoundBase> LightUpSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Power Down Sound"))
	TObjectPtr<USoundBase> PowerDownSound;

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnTransformerBushingStateChanged OnTransformerStateChanged;

	UFUNCTION(BlueprintNativeEvent, Category = "LiveWire|Gameplay")
 // OnLitStateChanged: on lit state changed
	void OnLitStateChanged(bool bNewLit);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
 // SetLitUp: set lit up
	void SetLitUp(bool bLit);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
 // Laser trace and collision
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	UFUNCTION()
 // OnTargetHitStateChanged: on target hit state changed
	void OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit);

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
 // ID color mapping and dynamic materials
	FLinearColor GetColorForID(int32 InID) const;

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
 // ID color mapping and dynamic materials
	FLinearColor GetActiveLitColor() const;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Appearance")
 // UpdateAppearance: update appearance
	void UpdateAppearance();

protected:
 // Called when the game starts or when spawned
	virtual void BeginPlay() override;
 // Editor construction script
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
 // Editor property change callback
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
 // ID color mapping and dynamic materials
	void SyncIDAndColor(int32 NewID);
 // BindTarget: bind target
	void BindTarget(ALaserTarget* Target);
 // BindToMatchingTargets: bind to matching targets
	void BindToMatchingTargets();
 // StartNeonIgnitionFlicker: start neon ignition flicker
	void StartNeonIgnitionFlicker();
 // SetMeshEmissive: set mesh emissive
	void SetMeshEmissive(const FLinearColor& Color);

	FTimerHandle FlickerTimerHandle;
	float TargetCheckTimer = 0.0f;
};
