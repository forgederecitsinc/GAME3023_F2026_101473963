// roshan - 101473963 - game3023 game engines 3
// live wire player character header
// fixed 2.5d imax top-down camera, wasd movement, and 1-slot battery inventory

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GAME3023_F26_RoshanCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class ABatteryCapsule;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

// main player character for the project
UCLASS(abstract)
class AGAME3023_F26_RoshanCharacter : public ACharacter
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
	// cinematic 2.5d imax camera settings
	// fixed top-down angle so the player gets a huge view of the arena
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraPitch = -60.0f; // pitch down looking at arena

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraYaw = 0.0f; // facing forward down x axis

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraDistance = 1750.0f; // arm length for wide area view

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraFOV = 85.0f; // wide imax field of view

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	float CameraLagSpeed = 4.0f; // smooth camera tracking speed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cinematic Camera")
	bool bEnableCameraCollision = false; // keep off so camera doesn't zoom in near catwalks

	// helper to lock down all camera settings
	UFUNCTION(BlueprintCallable, Category = "Cinematic Camera")
	void ApplyCinematicCameraSettings();

	// dualsense / gamepad controller settings so i can play and test with my ps5 controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableDualSenseController = true; // toggle controller support on/off

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float ControllerStickDeadZone = 0.2f; // deadzone so stick drift doesn't move the character

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableRightStickAiming = true; // twin-stick style aiming with right analog stick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller")
	bool bEnableControllerVibration = true; // rumble feedback when picking up or plugging in batteries

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DualSense Controller", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VibrationIntensity = 0.75f; // how strong the controller rumble feels

	// helper function to rumble the dualsense controller on actions
	UFUNCTION(BlueprintCallable, Category = "DualSense Controller")
	void TriggerControllerRumble(float Intensity = 0.6f, float Duration = 0.2f, bool bHeavyLeft = true, bool bLightRight = true);

	// override controller rotation inputs so mouse look does nothing to camera
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerRollInput(float Val) override;

public:

	AGAME3023_F26_RoshanCharacter();	

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// tracks if enhanced input already handled movement this frame so left stick doesn't double-add
	bool bMovedViaEnhancedInputThisFrame = false;

	// input bindings setup
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	// enhanced input callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

public:

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

public:

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
