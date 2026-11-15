// Student: Roshan - 101473963 | Course: GAME3023 - Game Engines 3
// Transformer Bushing Neon Indicator: lights up when a laser strikes a matching ALaserTarget.

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
	void OnLitStateChanged(bool bNewLit);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLitUp(bool bLit);

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	UFUNCTION()
	void OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit);

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetActiveLitColor() const;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Appearance")
	void UpdateAppearance();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void SyncIDAndColor(int32 NewID);
	void BindTarget(ALaserTarget* Target);
	void BindToMatchingTargets();
	void StartNeonIgnitionFlicker();
	void SetMeshEmissive(const FLinearColor& Color);

	FTimerHandle FlickerTimerHandle;
	float TargetCheckTimer = 0.0f;
};
