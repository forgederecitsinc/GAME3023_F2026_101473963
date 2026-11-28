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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Static Mesh Component"))
	TObjectPtr<UStaticMeshComponent> BushingMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (DisplayName = "Bushing Light"))
	TObjectPtr<UPointLightComponent> BushingLight;

	FLinearColor NeonLitColor = FLinearColor(0.00f, 0.95f, 1.00f);

	FLinearColor NeonUnlitColor = FLinearColor(0.04f, 0.04f, 0.05f);

	float LitEmissiveMultiplier = 25.0f;

	float UnlitEmissiveMultiplier = 0.0f;

	float LitLightIntensity = 8000.0f;

	float LightAttenuationRadius = 800.0f;

	bool bEnableNeonIgnitionFlicker = true;

	bool bPreviewLitInEditor = false;

	bool bIsLitUp = false;

	bool bStayLitOnceActivated = false;

	UPROPERTY()
	TObjectPtr<ALaserTarget> LinkedLaserTarget;

	UPROPERTY()
	TObjectPtr<USoundBase> LightUpSound;

	UPROPERTY()
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
