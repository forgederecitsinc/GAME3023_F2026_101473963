// roshan - 101473963 - game3023 game engines 3
// live wire player character header
// fixed 2.5d imax top-down camera, wasd movement, and 1-slot battery inventory

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CombatDamageable.h"

// Delegates for player health events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerHealthChanged, float, CurrentHealth, float, MaxHealth, float, DeltaHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);

#include "GAME3023_F26_RoshanCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class ABatteryCapsule;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMaterial;
class UPrimitiveComponent;

// struct to keep track of each roof/catwalk mesh that is currently hiding the player
USTRUCT()
struct FOccludingMeshData
{
	GENERATED_BODY()

	// original materials on the mesh so we can put them back when the player walks out
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	// dynamic cutout materials with the circular see-through mask
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CutoutMIDs;

	// current 0..1 fade alpha (smoothly lerps up when under roof, down when leaving)
	float CurrentAlpha = 0.0f;

	// whether the camera-to-player trace hit this mesh on the current frame
	bool bHitThisFrame = false;
};

class UInteractPromptWidget;
class UPlayerHealthHUDWidget;
class USoundBase;
class UAudioComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// main player character for the project
UCLASS(abstract)
class AGAME3023_F26_RoshanCharacter : public ACharacter, public ICombatDamageable
{
	GENERATED_BODY()

	// spring arm boom positioning the camera above the player
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	// camera component attached to boom
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	// enhanced input actions
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	// interact with E
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	// drop battery with R
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DropAction;

	// inventory: can only hold ONE battery capsule at a time (assignment rule)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<ABatteryCapsule> CarriedBattery;

	// actor we are standing close to (cylinder or battery)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<AActor> CurrentInteractable;

public:
	// hades style 2.5d top-down cinematic camera
	// fixed top angle looking down at arena so player gets an imax view and never gets dizzy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraPitch = -60.0f; // pitch down looking at arena (hades style)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraYaw = 0.0f; // facing forward down x axis

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera", meta = (ClampMin = "200.0", ClampMax = "4000.0"))
	float CameraDistance = 1600.0f; // arm length for wide arena view (editable in inspector)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	FVector CameraSocketOffset = FVector(0.0f, 0.0f, 100.0f); // slight height offset

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraFOV = 85.0f; // wide imax field of view

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraLagSpeed = 4.0f; // smooth camera crane tracking speed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	bool bEnableCameraCollision = false; // keep off so camera doesn't zoom in crazy when walking under catwalks

	// helper to lock down all camera settings
	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera")
	void ApplyCinematicCameraSettings();

	// dualsense / gamepad controller settings so i can play and test with my ps5 controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableDualSenseController = true; // toggle controller support on/off

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float ControllerStickDeadZone = 0.2f; // deadzone so stick drift doesn't move the character

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	float GamepadLookSensitivity = 1.5f; // dualsense right stick look speed for death stranding camera

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableRightStickAiming = true; // twin-stick style aiming fallback if in top-down mode

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableControllerVibration = true; // rumble feedback when picking up or plugging in batteries

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VibrationIntensity = 0.75f; // how strong the controller rumble feels

	// helper function to rumble the dualsense controller on actions
	UFUNCTION(BlueprintCallable, Category = "DualSense Controller")
	void TriggerControllerRumble(float Intensity = 0.6f, float Duration = 0.2f, bool bHeavyLeft = true, bool bLightRight = true);

	// occlusion cutout settings (hades / diablo style circular see-through lens when player walks under catwalks or roofs)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)")
	bool bEnableOcclusionCutout = true; // turn the roof see-through cutout on or off

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
	float OcclusionCutoutRadius = 260.0f; // size of the circular hole right above the player

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "10.0", ClampMax = "600.0"))
	float OcclusionEdgeSoftness = 140.0f; // how soft/feathered the border of the circle is

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float OcclusionFadeSpeed = 6.0f; // how smoothly the circle fades in and out when walking under a mesh

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float OcclusionMinCenterOpacity = 0.05f; // opacity right in the center above the player (0 = full cutout, 0.05 = slight glass feel)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)")
	FLinearColor OcclusionRimColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f); // subtle electric cyan hue on the cutout edge like hades/tunic

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OcclusionRimIntensity = 1.2f; // set to 0 if you want zero glow on the border

	// override controller rotation inputs so mouse look does nothing to camera
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerRollInput(float Val) override;

public:

	AGAME3023_F26_RoshanCharacter();	

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	// live update camera when changing settings in the editor inspector details panel
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// tracks if enhanced input already handled movement this frame so left stick doesn't double-add
	bool bMovedViaEnhancedInputThisFrame = false;

	// smoothed world position of the cutout center so the lens glides smoothly with the player
	FVector SmoothedOcclusionTargetPos = FVector::ZeroVector;

	// map of meshes currently fading in/out above the player
	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FOccludingMeshData> ActiveOccludingMeshes;

	// runs every tick to check if any roof or catwalk is between the camera and player
	void UpdateCameraOcclusionCutout(float DeltaTime);

	// input bindings setup
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	// enhanced input callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// disable jumping for the player character
	virtual bool CanJumpInternal_Implementation() const override;

public:

	// jump overrides to prevent jumping
	virtual void Jump() override;
	virtual void StopJumping() override;

	// wasd movement handling
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	// pressing E
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void DoInteract();

	// pressing R - drops battery on floor in front of player
	UFUNCTION(BlueprintCallable, Category="Inventory")
	virtual void DoDropBattery();

	// inventory helpers
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool TryPickupBattery(ABatteryCapsule* Battery);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	ABatteryCapsule* DropCurrentBattery();

	UFUNCTION(BlueprintCallable, Category="Inventory")
	ABatteryCapsule* TakeCarriedBattery();

	// check if holding a battery
	UFUNCTION(BlueprintPure, Category="Inventory")
	bool HasBattery() const { return CarriedBattery != nullptr; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	ABatteryCapsule* GetCarriedBattery() const { return CarriedBattery; }

	// getters and setters for current interactable
	void SetCurrentInteractable(AActor* Interactable) { CurrentInteractable = Interactable; }
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	// --- Interaction UI Widget ---

	// custom widget class to use for the interaction prompt (defaults to UInteractPromptWidget)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Interaction Prompt Widget Class"))
	TSubclassOf<UInteractPromptWidget> InteractionPromptWidgetClass;

	// active runtime interaction prompt widget instance
	UPROPERTY(Transient, BlueprintReadOnly, Category = "LiveWire|Interaction UI")
	TObjectPtr<UInteractPromptWidget> InteractionPromptWidget;

	// toggle to enable or disable the on-screen interaction prompt widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Show Interaction Widget"))
	bool bShowInteractionWidget = true;

	// radius around character to scan for nearby interactables (batteries, cylinders)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Interaction Detection Radius"))
	float InteractionDetectionRadius = 260.0f;

	// whether interaction prompt docks at bottom-center of screen (recommended for 2.5d top-down)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Anchor Prompt To Screen Bottom"))
	bool bAnchorPromptToScreenBottom = true;

	// initialize and spawn the interaction prompt widget on the viewport
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
	void InitInteractionPromptWidget();

	// updates interaction prompt visibility and text based on nearby interactable actors
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
	void UpdateInteractionPrompt(float DeltaTime);

	// ==========================================
	// LiveWire Player Health HUD Widget (Top-Right)
	// ==========================================

	// custom widget class to use for player health HUD (defaults to UPlayerHealthHUDWidget)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health HUD", meta = (DisplayName = "Health HUD Widget Class"))
	TSubclassOf<UPlayerHealthHUDWidget> HealthHUDWidgetClass;

	// active runtime player health HUD widget instance (anchored to top-right of screen)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "LiveWire|Health HUD")
	TObjectPtr<UPlayerHealthHUDWidget> HealthHUDWidget;

	// toggle to enable or disable the on-screen top-right player health HUD
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health HUD", meta = (DisplayName = "Show Health HUD"))
	bool bShowHealthHUD = true;

	// initialize and spawn player health HUD widget on the viewport
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
	void InitHealthHUDWidget();

	// updates the player health HUD widget with current health points
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
	void UpdateHealthHUD();

	// ==========================================
	// LiveWire Background Music (BGM)
	// ==========================================

	// Background music track asset to play (defaults to /Game/Sound/LiveWire_Substation_Theme)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Background Music Track"))
	TObjectPtr<USoundBase> BackgroundMusicTrack;

	// Volume multiplier for the background music (0.0 = silent, 1.0 = normal, clamp 0 to 2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "BGM Volume", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "1.0"))
	float BGMVolume = 0.65f;

	// Pitch multiplier for the background music (1.0 = normal speed/pitch)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "BGM Pitch", ClampMin = "0.2", ClampMax = "2.0"))
	float BGMPitch = 1.0f;

	// Whether background music starts playing automatically the moment player enters the level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Auto-Play BGM On Start"))
	bool bAutoPlayBGM = true;

	// Starts playing the background music track (loops continuously)
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Audio")
	void PlayBackgroundMusic();

	// Stops the background music track
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Audio")
	void StopBackgroundMusic();

	// Sets the volume of the background music at runtime
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Audio")
	void SetBGMVolume(float NewVolume);

	// Called automatically when the BGM track reaches the end to loop it seamlessly
	UFUNCTION()
	void OnBGMAudioFinished();

	// Currently active 2D audio component playing the background music
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Audio", meta = (DisplayName = "Active BGM Audio Component"))
	TObjectPtr<UAudioComponent> BGMAudioComponent;

	// ==========================================
	// LiveWire Player Health System
	// ==========================================

	// Maximum health points for the player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (ClampMin = "1.0", DisplayName = "Max Health"))
	float MaxHealth = 100.0f;

	// Current health points for the player (starts at 100 when game begins)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (DisplayName = "Current Health"))
	float CurrentHealth = 100.0f;

	// Whether the player is currently dead
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Health", meta = (DisplayName = "Is Dead"))
	bool bIsDead = false;

	// Event fired when player health changes (CurrentHealth, MaxHealth, DeltaHealth)
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Health")
	FOnPlayerHealthChanged OnHealthChanged;

	// Event fired when player health reaches zero
	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Health")
	FOnPlayerDied OnPlayerDied;

	// Standard Unreal Engine TakeDamage override
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// ICombatDamageable interface implementation
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	// Deals damage to player and updates health points
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void ApplyDamageToPlayer(float DamageAmount, AActor* DamageCauser = nullptr);

	// Heals the player by a specified amount up to MaxHealth
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void HealPlayer(float HealAmount);

	// Directly sets the player's health points
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void SetHealth(float NewHealth);

	// Resets player health back to MaxHealth (100) and revives player
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void ResetHealth();

	// Health getters
	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	bool IsDead() const { return bIsDead; }

	// Convenience HP aliases
	UFUNCTION(BlueprintPure, Category = "LiveWire|Health", meta = (DisplayName = "Get Current HP"))
	float GetCurrentHP() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health", meta = (DisplayName = "Get Max HP"))
	float GetMaxHP() const { return MaxHealth; }

	// Debug key actions to test damage and healing on the health HUD
	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void TestTakeDamage();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void TestHeal();

public:

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
