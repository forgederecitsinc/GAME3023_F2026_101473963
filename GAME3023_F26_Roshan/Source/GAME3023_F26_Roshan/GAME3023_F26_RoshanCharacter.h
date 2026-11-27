#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "CombatDamageable.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerHealthChanged, float, CurrentHealth, float, MaxHealth, float, DeltaHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerGameOver);

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

USTRUCT()
struct FOccludingMeshData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CutoutMIDs;

	float CurrentAlpha = 0.0f;

	bool bHitThisFrame = false;
};

class UInteractPromptWidget;
class UPlayerHealthHUDWidget;
class USoundBase;
class UAudioComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// Main player character: handles movement, interaction, inventory, health, and camera behavior.
UCLASS(abstract)
class AGAME3023_F26_RoshanCharacter : public ACharacter, public ICombatDamageable
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DropAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<ABatteryCapsule> CarriedBattery;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<AActor> CurrentInteractable;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraPitch = -60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera", meta = (ClampMin = "200.0", ClampMax = "4000.0"))
	float CameraDistance = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	FVector CameraSocketOffset = FVector(0.0f, 0.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraFOV = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraLagSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	bool bEnableCameraCollision = false;

	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera")
 // ApplyCinematicCameraSettings: apply cinematic camera settings
	void ApplyCinematicCameraSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableDualSenseController = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float ControllerStickDeadZone = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	float GamepadLookSensitivity = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableRightStickAiming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bInvertRightStickAimY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableControllerVibration = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VibrationIntensity = 0.75f;

	UFUNCTION(BlueprintCallable, Category = "DualSense Controller")
 // TriggerControllerRumble: trigger controller rumble
	void TriggerControllerRumble(float Intensity = 0.6f, float Duration = 0.2f, bool bHeavyLeft = true, bool bLightRight = true);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)")
	bool bEnableOcclusionCutout = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
	float OcclusionCutoutRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "10.0", ClampMax = "600.0"))
	float OcclusionEdgeSoftness = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float OcclusionFadeSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "0.0", ClampMax = "0.8"))
	float OcclusionMinCenterOpacity = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)")
	FLinearColor OcclusionRimColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Occlusion Cutout (See-Through)", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float OcclusionRimIntensity = 1.2f;

 // AddControllerYawInput: add controller yaw input
	virtual void AddControllerYawInput(float Val) override;
 // AddControllerPitchInput: add controller pitch input
	virtual void AddControllerPitchInput(float Val) override;
 // AddControllerRollInput: add controller roll input
	virtual void AddControllerRollInput(float Val) override;

public:

	AGAME3023_F26_RoshanCharacter();	

protected:
	// Runs when the player character starts or respawns.
	virtual void BeginPlay() override;
	// Runs when a player controller takes control of this character.
	virtual void PossessedBy(AController* NewController) override;
	// Runs when the character leaves the level.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	// Runs every frame to update camera, interaction, and HUD behavior.
	virtual void Tick(float DeltaTime) override;
	// Updates character settings while it is placed or changed in the editor.
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
 // Editor property change callback
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	bool bMovedViaEnhancedInputThisFrame = false;

	FVector SmoothedOcclusionTargetPos = FVector::ZeroVector;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UPrimitiveComponent>, FOccludingMeshData> ActiveOccludingMeshes;

 // UpdateCameraOcclusionCutout: update camera occlusion cutout
	void UpdateCameraOcclusionCutout(float DeltaTime);

 // Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

 // Move: move
	void Move(const FInputActionValue& Value);
 // Look: look
	void Look(const FInputActionValue& Value);

 // CanJumpInternal_Implementation: can jump internal implementation
	virtual bool CanJumpInternal_Implementation() const override;

public:

 // Jump: jump
	virtual void Jump() override;
 // StopJumping: stop jumping
	virtual void StopJumping() override;

	UFUNCTION(BlueprintCallable, Category="Input")
 // DoMove: do move
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
 // DoLook: do look
	virtual void DoLook(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category="Input")
 // DoJumpStart: do jump start
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category="Input")
 // DoJumpEnd: do jump end
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Interaction")
 // Handles DoInteract interaction
	virtual void DoInteract();

	UFUNCTION(BlueprintCallable, Category="Interaction")
 // Handles DoInteractRelease interaction
	virtual void DoInteractRelease();

	UFUNCTION(BlueprintPure, Category="Interaction")
 // Battery state and handling
	float GetBatteryAttachHoldProgress() const;

 // Handles IsInteractInputHeld interaction
	bool IsInteractInputHeld() const;

 // Battery state and handling
	void ResetBatteryAttachHold();

	UFUNCTION(BlueprintCallable, Category="Inventory")
 // Battery state and handling
	virtual void DoDropBattery();

	UFUNCTION(BlueprintCallable, Category="Inventory")
 // Battery state and handling
	bool TryPickupBattery(ABatteryCapsule* Battery);

	UFUNCTION(BlueprintCallable, Category="Inventory")
 // Battery state and handling
	ABatteryCapsule* DropCurrentBattery();

	UFUNCTION(BlueprintCallable, Category="Inventory")
 // Battery state and handling
	ABatteryCapsule* TakeCarriedBattery();

	UFUNCTION(BlueprintPure, Category="Inventory")
 // Battery state and handling
	bool HasBattery() const { return CarriedBattery != nullptr; }

	UFUNCTION(BlueprintPure, Category="Inventory")
 // Battery state and handling
	ABatteryCapsule* GetCarriedBattery() const { return CarriedBattery; }

 // Handles SetCurrentInteractable interaction
	void SetCurrentInteractable(AActor* Interactable) { CurrentInteractable = Interactable; }
 // Handles GetCurrentInteractable interaction
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Interaction Prompt Widget Class"))
	TSubclassOf<UInteractPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "LiveWire|Interaction UI")
	TObjectPtr<UInteractPromptWidget> InteractionPromptWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Show Interaction Widget"))
	bool bShowInteractionWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Interaction Detection Radius"))
	float InteractionDetectionRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Anchor Prompt To Screen Bottom"))
	bool bAnchorPromptToScreenBottom = true;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
 // Handles InitInteractionPromptWidget interaction
	void InitInteractionPromptWidget();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
 // Handles UpdateInteractionPrompt interaction
	void UpdateInteractionPrompt(float DeltaTime);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Interaction UI", meta = (ClampMin = "0.5", DisplayName = "Battery Attach Hold Duration (Seconds)"))
	float BatteryAttachHoldDuration = 5.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Current Attach Hold Time"))
	float CurrentAttachHoldTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Interaction UI", meta = (DisplayName = "Is Holding To Attach Battery"))
	bool bIsHoldingToAttach = false;

protected:
	UPROPERTY(Transient)
	TObjectPtr<class ARedPowerCylinder> HoldingTargetCylinder = nullptr;

	bool bEnhancedInteractHeld = false;
	bool bRawInteractKeyHeld = false;

	bool bRequireFreshPressForHold = false;

	uint64 LastInteractPressFrame = 0;

	float HoldRumblePulseTimer = 0.0f;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health HUD", meta = (DisplayName = "Health HUD Widget Class"))
	TSubclassOf<UPlayerHealthHUDWidget> HealthHUDWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "LiveWire|Health HUD")
	TObjectPtr<UPlayerHealthHUDWidget> HealthHUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health HUD", meta = (DisplayName = "Show Health HUD"))
	bool bShowHealthHUD = true;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
 // Health and damage management
	void InitHealthHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
 // Health and damage management
	void UpdateHealthHUD();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Background Music Track"))
	TObjectPtr<USoundBase> BackgroundMusicTrack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "BGM Volume", ClampMin = "0.0", ClampMax = "2.0", UIMin = "0.0", UIMax = "1.0"))
	float BGMVolume = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "BGM Pitch", ClampMin = "0.2", ClampMax = "2.0"))
	float BGMPitch = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Audio", meta = (DisplayName = "Auto-Play BGM On Start"))
	bool bAutoPlayBGM = true;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Audio")
 // PlayBackgroundMusic: play background music
	void PlayBackgroundMusic();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Audio")
 // StopBackgroundMusic: stop background music
	void StopBackgroundMusic();

	UFUNCTION()
 // OnBGMAudioFinished: on bgm audio finished
	void OnBGMAudioFinished();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Audio", meta = (DisplayName = "Active BGM Audio Component"))
	TObjectPtr<UAudioComponent> BGMAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (ClampMin = "1.0", DisplayName = "Max Health"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (DisplayName = "Current Health"))
	float CurrentHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LiveWire|Health", meta = (DisplayName = "Is Dead"))
	bool bIsDead = false;

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Health")
	FOnPlayerHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Health")
	FOnPlayerDied OnPlayerDied;

 // Health and damage management
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

 // Health and damage management
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
 // Death and elimination handling
	virtual void HandleDeath() override;
 // ApplyHealing: apply healing
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
 // NotifyDanger: notify danger
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
 // Health and damage management
	void ApplyDamageToPlayer(float DamageAmount, AActor* DamageCauser = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (DisplayName = "Respawn Location"))
	FVector RespawnLocation = FVector(56.0f, 0.0f, 745.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (ClampMin = "0.0", ClampMax = "10.0", DisplayName = "Respawn Delay (Seconds)"))
	float RespawnDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (ClampMin = "1", DisplayName = "Max Lives"))
	int32 MaxLives = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (DisplayName = "Remaining Lives"))
	int32 RemainingLives = 3;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "LiveWire|Health", meta = (DisplayName = "Is Game Over"))
	bool bIsGameOver = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiveWire|Health", meta = (ClampMin = "0.0", DisplayName = "Game Over Quit Delay (Seconds)"))
	float GameOverQuitDelay = 2.5f;

	UPROPERTY(BlueprintAssignable, Category = "LiveWire|Health")
	FOnPlayerGameOver OnGameOver;

	FTimerHandle RespawnTimerHandle;
	FTimerHandle GameOverTimerHandle;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
	void HandleGameOver();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	int32 GetRemainingLives() const { return RemainingLives; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	int32 GetMaxLives() const { return MaxLives; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
	bool IsGameOver() const { return bIsGameOver; }

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health")
 // Spawning logic and management
	void RespawnPlayer();

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
 // Health and damage management
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
 // Health and damage management
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
 // Health and damage management
	float GetHealthPercent() const { return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "LiveWire|Health")
 // IsDead: is dead
	bool IsDead() const { return bIsDead; }

public:

 // GetCameraBoom: get camera boom
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
 // GetFollowCamera: get follow camera
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:
 // UpdateDualSenseControls: update dual sense controls
	void UpdateDualSenseControls(float DeltaTime);
 // AcquireOccludingMesh: acquire occluding mesh
	void AcquireOccludingMesh(class UPrimitiveComponent* Comp);
 // FadeOccludingMeshes: fade occluding meshes
	void FadeOccludingMeshes(float DeltaTime, const FVector& CameraLoc);
 // Handles InteractWithCylinder interaction
	bool InteractWithCylinder(class ARedPowerCylinder* Cylinder);
 // Handles FindBestInteractable interaction
	AActor* FindBestInteractable();
 // UpdateCylinderHoldPrompt: update cylinder hold prompt
	void UpdateCylinderHoldPrompt(class ARedPowerCylinder* Cylinder, float DeltaTime, bool bInputHeld, FString& OutAction, FString& OutTarget);
 // Health and damage management
	void ApplyHealthChange(float NewHealth);
};
