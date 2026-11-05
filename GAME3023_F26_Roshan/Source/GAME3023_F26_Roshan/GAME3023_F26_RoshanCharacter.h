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
