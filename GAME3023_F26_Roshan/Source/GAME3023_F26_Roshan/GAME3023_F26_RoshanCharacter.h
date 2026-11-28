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

	UPROPERTY()
	UInputAction* JumpAction;

	UPROPERTY()
	UInputAction* MoveAction;

	UPROPERTY()
	UInputAction* LookAction;

	UPROPERTY()
	UInputAction* MouseLookAction;

	UPROPERTY()
	UInputAction* InteractAction;

	UPROPERTY()
	UInputAction* DropAction;

	UPROPERTY()
	TObjectPtr<ABatteryCapsule> CarriedBattery;

	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;

public:
	float CameraPitch = -60.0f;

	float CameraYaw = 0.0f;

	float CameraDistance = 1600.0f;

	FVector CameraSocketOffset = FVector(0.0f, 0.0f, 100.0f);

	float CameraFOV = 85.0f;

	float CameraLagSpeed = 4.0f;

	bool bEnableCameraCollision = false;

	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera")
 // ApplyCinematicCameraSettings: apply cinematic camera settings
	void ApplyCinematicCameraSettings();

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

	UPROPERTY()
	TSubclassOf<UInteractPromptWidget> InteractionPromptWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UInteractPromptWidget> InteractionPromptWidget;

	bool bShowInteractionWidget = true;

	float InteractionDetectionRadius = 260.0f;

	bool bAnchorPromptToScreenBottom = true;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
 // Handles InitInteractionPromptWidget interaction
	void InitInteractionPromptWidget();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Interaction UI")
 // Handles UpdateInteractionPrompt interaction
	void UpdateInteractionPrompt(float DeltaTime);

public:

	UPROPERTY()
	TSubclassOf<UPlayerHealthHUDWidget> HealthHUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerHealthHUDWidget> HealthHUDWidget;

	bool bShowHealthHUD = true;

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
 // Health and damage management
	void InitHealthHUDWidget();

	UFUNCTION(BlueprintCallable, Category = "LiveWire|Health HUD")
 // Health and damage management
	void UpdateHealthHUD();

	UPROPERTY()
	TObjectPtr<USoundBase> BackgroundMusicTrack;

	float BGMVolume = 0.65f;

	float BGMPitch = 1.0f;

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

	float MaxHealth = 100.0f;

	float CurrentHealth = 100.0f;

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

	FVector RespawnLocation = FVector(56.0f, 0.0f, 745.0f);

	float RespawnDelay = 1.0f;

	int32 MaxLives = 3;

	int32 RemainingLives = 3;

	bool bIsGameOver = false;

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
 // Handles InteractWithCylinder interaction
	bool InteractWithCylinder(class ARedPowerCylinder* Cylinder);
 // Handles FindBestInteractable interaction
	AActor* FindBestInteractable();
 // Health and damage management
	void ApplyHealthChange(float NewHealth);
};
