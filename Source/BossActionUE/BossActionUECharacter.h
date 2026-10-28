// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"

#include "BossActionUECharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UBossHealthComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
enum class EPlayerHitResult : uint8
{
	Ignored,
	Damaged,
	Parried,
	Evaded
};
UCLASS(abstract)
class ABossActionUECharacter : public ACharacter
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

	/** Attack Input Action */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* AttackAction = nullptr;

	/** Distance from the character to the attack sphere center, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackForwardOffset = 150.f;

	/** Attack sphere radius, in cm */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1.0"))
	float AttackRadius = 120.f;

	/** Damage applied by one attack */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackDamage = 25.f;

	/** Time spent in the attacking state */
	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float AttackDuration = 0.5f;

	/** Whether an attack is currently in progress */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat")
	bool bIsAttacking = false;

	/** Handle used to manage the attack timer */
	FTimerHandle AttackTimerHandle;

	/** Player health component */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBossHealthComponent* HealthComponent;

	/** Temporary input for testing player damage */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* TestDamageAction = nullptr;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	void TestDamage();

	UFUNCTION()
	void HandleDeath();

public:

	/** Constructor */
	ABossActionUECharacter();	

	/** Entry point for damage received by the player */
	EPlayerHitResult ReceiveHit(float DamageAmount);

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called when attack input starts */
	void Attack();

	void FinishAttack();

	/** Parry input */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ParryAction = nullptr;

	/** Time during which incoming damage can be parried */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Parry", meta = (ClampMin = "0.01"))
	float ParryDuration = 0.2f;

	/** Cooldown measured from the start of the parry */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Parry", meta = (ClampMin = "0.01"))
	float ParryCooldown = 0.8f;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Parry")
	bool bIsParrying = false;

	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Parry")
	bool bParryOnCooldown = false;

	FTimerHandle ParryTimerHandle;
	FTimerHandle ParryCooldownTimerHandle;

	void StartParry();
	void EndParry();
	void FinishParryCooldown();
	/** 구르기 입력 액션 */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* RollAction = nullptr;

	/** 구르기 지속 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Roll", meta = (ClampMin = "0.01"))
	float RollDuration = 0.25f;

	/** 구르기 시작 시점부터 계산하는 쿨타임 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Roll", meta = (ClampMin = "0.01"))
	float RollCooldown = 10.f;

	/** 구르기 이동 속도: cm/s */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Roll", meta = (ClampMin = "1.0"))
	float RollSpeed = 1200.f;

	/** 현재 구르는 중인지 */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Roll")
	bool bIsRolling = false;

	/** 구르기 쿨타임 중인지 */
	UPROPERTY(VisibleInstanceOnly, Category = "Combat|Roll")
	bool bRollOnCooldown = false;

	/** 구르기를 시작할 때 결정한 방향 */
	FVector RollDirection = FVector::ZeroVector;

	/** 구르기 종료 후 복원할 이동 속도 */
	float SavedMaxWalkSpeed = 0.f;

	/** 구르기 종료 타이머 */
	FTimerHandle RollTimerHandle;

	/** 구르기 쿨타임 타이머 */
	FTimerHandle RollCooldownTimerHandle;

	void StartRoll();
	void FinishRoll();
	void FinishRollCooldown();

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

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

