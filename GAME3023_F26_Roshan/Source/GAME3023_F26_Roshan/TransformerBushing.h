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
	// Constructor: creates the bushing mesh and point light.
	ATransformerBushing();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Static Mesh Component"))
	TObjectPtr<UStaticMeshComponent> BushingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Bushing Light"))
	TObjectPtr<UPointLightComponent> BushingLight;

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

	// Delegate broadcast when the bushing turns on or off.
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnTransformerBushingStateChanged OnTransformerStateChanged;

	// Blueprint event called when the lit state changes.
	UFUNCTION(BlueprintNativeEvent, Category = "LiveWire|Gameplay")
	void OnLitStateChanged(bool bNewLit);

	// Turns the neon bushing on or off.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLitUp(bool bLit);

	// Compatibility function that uses the same behavior as SetLitUp.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	// Runs when a linked laser target changes its hit state.
	UFUNCTION()
	void OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit);

	// Returns the color currently used by the lit bushing.
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetActiveLitColor() const;

	// Updates the mesh, light, and emissive appearance.
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Appearance")
	void UpdateAppearance();

protected:
	// Runs when the game starts or the bushing is spawned.
	virtual void BeginPlay() override;

	// Updates the bushing while it is placed or changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// Updates the bushing after an editor property is changed.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	// Plays a short flicker effect when the bushing turns on.
	void StartNeonIgnitionFlicker();

	// Changes the emissive color on the bushing mesh material.
	void SetMeshEmissive(const FLinearColor& Color);

	FTimerHandle FlickerTimerHandle;
};
