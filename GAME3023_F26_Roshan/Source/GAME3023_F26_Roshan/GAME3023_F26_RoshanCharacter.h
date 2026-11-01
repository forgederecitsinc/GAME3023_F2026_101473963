// Copyright Epic Games, Inc. All Rights Reserved.

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

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AGAME3023_F26_RoshanCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Interact Input Action (Press E) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	/** Currently carried battery capsule in inventory (max 1) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<ABatteryCapsule> CarriedBattery;

	/** Overlapping interactive actor in range */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction")
	TObjectPtr<AActor> CurrentInteractable;

public:

	/** Constructor */
	AGAME3023_F26_RoshanCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles interaction input (Press E) */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	virtual void DoInteract();

	/** Attempts to pick up a battery capsule into inventory (fails if already carrying one) */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool TryPickupBattery(ABatteryCapsule* Battery);

	/** Drops the currently carried battery */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	ABatteryCapsule* DropCurrentBattery();

	/** Transfers the carried battery out of inventory (e.g. to socket into a cell) */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	ABatteryCapsule* TakeCarriedBattery();

	/** Checks if character is currently carrying a battery */
	UFUNCTION(BlueprintPure, Category="Inventory")
	bool HasBattery() const { return CarriedBattery != nullptr; }

	/** Gets the currently carried battery */
	UFUNCTION(BlueprintPure, Category="Inventory")
	ABatteryCapsule* GetCarriedBattery() const { return CarriedBattery; }

	/** Sets or clears current interactable actor */
	void SetCurrentInteractable(AActor* Interactable) { CurrentInteractable = Interactable; }
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

