// roshan - 101473963 - game3023 game engines 3
// transformer bushing header - lights up like a neon light when the matching target is hit
// supports custom user-chosen neon colors and authentic neon tube bloom

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TransformerBushing.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UPointLightComponent;
class UMaterialInterface;
class USoundBase;
class ALaserTarget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTransformerBushingStateChanged, class ATransformerBushing*, Bushing, bool, bIsLit);

UCLASS()
class GAME3023_F26_ROSHAN_API ATransformerBushing : public AActor
{
	GENERATED_BODY()
	
public:	
	ATransformerBushing();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	virtual void Tick(float DeltaTime) override;

	// ==========================================
	// ID Matching
	// ==========================================

	// Bushing 4-digit ID (1001 = Red, 1002 = Green, 1003 = Blue, 1004 = Yellow, etc.)
	// Lights up when the laser hits the target that has the same ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Bushing ID (4-Digit)"))
	int32 BushingID = 1001;

	// Alias for TargetID for reflection compatibility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|ID", meta = (DisplayName = "Target ID (Alias)"))
	int32 TargetID = 1001;

	// ==========================================
	// Components
	// ==========================================

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	// Static mesh component - assign any mesh here in BP_TransformerBushing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Static Mesh Component"))
	TObjectPtr<UStaticMeshComponent> BushingMesh;

	// Point light that casts neon illumination into the surrounding area
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (DisplayName = "Bushing Light"))
	TObjectPtr<UPointLightComponent> BushingLight;

	// ==========================================
	// Neon Light & Color Customization
	// ==========================================

	// If true, uses the custom NeonLitColor chosen below. If false, automatically uses the palette color for BushingID.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Use Custom Neon Color"))
	bool bUseCustomNeonColor = true;

	// The color the mesh lights up to when the target ID matches! (Choose any color via the color picker)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Lit Color"))
	FLinearColor NeonLitColor = FLinearColor(0.00f, 0.95f, 1.00f); // Vibrant neon electric cyan

	// Unlit idle color (dark inactive glass/tube appearance when turned off)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Unlit Color"))
	FLinearColor NeonUnlitColor = FLinearColor(0.04f, 0.04f, 0.05f);

	// Emissive multiplier applied when lit up (values around 20-30 produce intense neon bloom in Unreal)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Neon Emissive Multiplier", ClampMin = "1.0", ClampMax = "100.0"))
	float LitEmissiveMultiplier = 25.0f;

	// Emissive multiplier applied when unlit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Unlit Emissive Multiplier"))
	float UnlitEmissiveMultiplier = 0.0f;

	// Point light intensity in lumens when lit up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Lit Light Intensity (Lumens)"))
	float LitLightIntensity = 8000.0f;

	// Attenuation radius for neon illumination in cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Light Radius"))
	float LightAttenuationRadius = 800.0f;

	// Whether to simulate authentic neon tube ignition flicker when turning on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Enable Neon Ignition Flicker"))
	bool bEnableNeonIgnitionFlicker = true;

	// Preview the lit-up neon look right inside the editor viewport without hitting Play
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Neon Appearance", meta = (DisplayName = "Preview Neon Lit in Editor"))
	bool bPreviewLitInEditor = false;

	// ==========================================
	// Materials
	// ==========================================

	// Whether to swap to NeonMaterial when lit up and UnlitMaterial when off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Swap Materials For Neon"))
	bool bSwapMaterialsForNeon = true;

	// Glowing emissive material used when lit up (default: M_laserbeam)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Neon Material (Lit)"))
	TObjectPtr<UMaterialInterface> NeonMaterial;

	// Base material used when unlit (default: M_FlatCol)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Unlit Material (Off)"))
	TObjectPtr<UMaterialInterface> UnlitMaterial;

	// Legacy compatibility alias
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Materials", meta = (DisplayName = "Base Material (Fallback)"))
	TObjectPtr<UMaterialInterface> BaseMaterial;

	// ==========================================
	// Gameplay & State
	// ==========================================

	// Whether the transformer bushing is currently lit up
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Gameplay", meta = (DisplayName = "Is Lit Up"))
	bool bIsLitUp = false;

	// If true, the transformer stays permanently lit once triggered
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Stay Lit Once Activated"))
	bool bStayLitOnceActivated = false;

	// Optional direct reference to a Laser Target in the level (if null, automatically binds by ID)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "LiveWire|Gameplay", meta = (DisplayName = "Linked Laser Target (Optional)"))
	TObjectPtr<ALaserTarget> LinkedLaserTarget;

	// Optional sound to play when lighting up
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Light Up Sound"))
	TObjectPtr<USoundBase> LightUpSound;

	// Optional sound to play when turning off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Power Down Sound"))
	TObjectPtr<USoundBase> PowerDownSound;

	// Multicast delegate fired when lit up state changes
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Gameplay")
	FOnTransformerBushingStateChanged OnTransformerStateChanged;

	// Blueprint native event for custom Blueprint responses
	UFUNCTION(BlueprintNativeEvent, Category = "LiveWire|Gameplay")
	void OnLitStateChanged(bool bNewLit);

	// ==========================================
	// Public API
	// ==========================================

	// Set whether this bushing is lit up
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLitUp(bool bLit);

	// Handle direct laser hit
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Gameplay")
	void SetLaserHit(bool bHit, AActor* SourceActor = nullptr);

	// Callback when linked/matching laser target hit state changes
	UFUNCTION()
	void OnTargetHitStateChanged(ALaserTarget* Target, bool bIsHit);

	// Unified 4-digit ID color conversion
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetColorForID(int32 InID) const;

	// Returns the active neon color (either the user-chosen NeonLitColor or the ID palette color)
	UFUNCTION(BlueprintPure, Category = "LiveWire|Color")
	FLinearColor GetActiveLitColor() const;

	// Updates dynamic material parameters, neon glow, and point light
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Appearance")
	void UpdateAppearance();

private:
	void BindToMatchingTargets();
	void StartNeonIgnitionFlicker();

	FTimerHandle FlickerTimerHandle;
};
