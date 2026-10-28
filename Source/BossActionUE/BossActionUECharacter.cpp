// Copyright Epic Games, Inc. All Rights Reserved.

#include "BossActionUECharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "BossActionUE.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "BossHealthComponent.h"
#include "GameFramework/PlayerController.h"

ABossActionUECharacter::ABossActionUECharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UBossHealthComponent>(TEXT("HealthComponent"));

	PrimaryActorTick.bCanEverTick = true;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ABossActionUECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABossActionUECharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABossActionUECharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ABossActionUECharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABossActionUECharacter::Look);

		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(
				AttackAction,
				ETriggerEvent::Started,
				this,
				&ABossActionUECharacter::Attack
			);
		}
		else
		{
			UE_LOG(LogBossActionUE, Warning,
				TEXT("AttackAction is not assigned on %s"), *GetName());
		}
	
		if (TestDamageAction)
		{
			EnhancedInputComponent->BindAction(
				TestDamageAction,
				ETriggerEvent::Started,
				this,
				&ABossActionUECharacter::TestDamage
			);
		}
		else
		{
			UE_LOG(
				LogBossActionUE,
				Warning,
				TEXT("TestDamageAction is not assigned on %s"),
				*GetName()
			);
		}
		// Parry input
		if (ParryAction)
		{
			EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Started, this, &ABossActionUECharacter::StartParry);
		}
		else
		{
			UE_LOG(LogBossActionUE, Warning, TEXT("ParryAction is not assigned on %s"), *GetName());
		}
		if (RollAction)
		{
			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &ABossActionUECharacter::StartRoll);
		}
		else
		{
			UE_LOG(LogBossActionUE, Warning, TEXT("RollAction is not assigned on %s"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogBossActionUE, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABossActionUECharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void ABossActionUECharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void ABossActionUECharacter::DoMove(float Right, float Forward)
{
	if (bIsRolling)
	{
		return;
	}
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void ABossActionUECharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ABossActionUECharacter::DoJumpStart()
{
	if (bIsRolling)
	{
		return;
	}

	Jump();
}

void ABossActionUECharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void ABossActionUECharacter::Attack()
{
	if (bIsRolling)
	{
		return;
	}
	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}
	if (bIsParrying)
	{
		return;
	}
	// Reject additional input while an attack is in progress
	if (bIsAttacking)
	{
		UE_LOG(LogBossActionUE, Log, TEXT("Attack ignored: already attacking"));
		return;
	}

	bIsAttacking = true;

	GetWorldTimerManager().SetTimer(
		AttackTimerHandle,
		this,
		&ABossActionUECharacter::FinishAttack,
		FMath::Max(AttackDuration, 0.01f),
		false
	);

	UE_LOG(LogBossActionUE, Log, TEXT("Attack started"));

	const FVector AttackCenter = GetActorLocation() + GetActorForwardVector() * AttackForwardOffset;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(
		UEngineTypes::ConvertToObjectType(ECC_Pawn)
	);
	ObjectTypes.Add(
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic)
	);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<AActor*> HitActors;

	UKismetSystemLibrary::SphereOverlapActors(
		this,
		AttackCenter,
		AttackRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		HitActors
	);

	int32 DamagedTargetCount = 0;

	for (AActor* HitActor : HitActors)
	{
		if (!IsValid(HitActor))
		{
			continue;
		}

		UBossHealthComponent* TargetHealthComponent = HitActor->FindComponentByClass<UBossHealthComponent>();

		if (!IsValid(TargetHealthComponent) || TargetHealthComponent->IsDead() || AttackDamage <= 0.f)
		{
			continue;
		}

		TargetHealthComponent->ReceiveDamage(AttackDamage);
		++DamagedTargetCount;
	}

	const FColor DebugColor =
		DamagedTargetCount > 0 ? FColor::Green : FColor::Red;

	DrawDebugSphere(
		GetWorld(),
		AttackCenter,
		AttackRadius,
		24,
		DebugColor,
		false,
		1.0f,
		0,
		2.0f
	);

	UE_LOG(
		LogBossActionUE,
		Log,
		TEXT("Attack damaged %d target(s)"),
		DamagedTargetCount
	);
}

void ABossActionUECharacter::FinishAttack()
{
	bIsAttacking = false;

	UE_LOG(LogBossActionUE, Log, TEXT("Attack finished"));
}

void ABossActionUECharacter::BeginPlay()
{
	Super::BeginPlay();

	HealthComponent->OnDeath.AddDynamic(
		this,
		&ABossActionUECharacter::HandleDeath
	);
}

void ABossActionUECharacter::TestDamage()
{
	ReceiveHit(25.f);
}

EPlayerHitResult ABossActionUECharacter::ReceiveHit(float DamageAmount)
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return EPlayerHitResult::Ignored;
	}

	if (DamageAmount <= 0.f)
	{
		return EPlayerHitResult::Ignored;
	}

	if (bIsRolling)
	{
		UE_LOG(LogBossActionUE, Log, TEXT("Roll evaded hit"));
		return EPlayerHitResult::Evaded;
	}

	if (bIsParrying)
	{
		UE_LOG(LogBossActionUE, Log, TEXT("Parry success"));
		EndParry();

		return EPlayerHitResult::Parried;
	}

	HealthComponent->ReceiveDamage(DamageAmount);

	return EPlayerHitResult::Damaged;
}

void ABossActionUECharacter::HandleDeath()
{
	FinishRoll();
	GetWorldTimerManager().ClearTimer(RollCooldownTimerHandle);
	bRollOnCooldown = false;
	// Cancel the current attack
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	bIsAttacking = false;

	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryCooldownTimerHandle);

	bIsParrying = false;
	bParryOnCooldown = false;

	// Stop movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// Stop this character from receiving player input
	if (APlayerController* PlayerController =
		Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}

	UE_LOG(LogBossActionUE, Log, TEXT("Player died"));
}
void ABossActionUECharacter::StartParry()
{
	if (bIsRolling)
	{
		return;
	}
	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}

	if (bIsAttacking || bIsParrying || bParryOnCooldown)
	{
		UE_LOG(LogBossActionUE, Log, TEXT("Parry unavailable"));
		return;
	}

	bIsParrying = true;
	bParryOnCooldown = true;

	const float WindowDuration = FMath::Max(ParryDuration, 0.01f);
	const float CooldownDuration = FMath::Max(ParryCooldown, 0.01f);

	GetWorldTimerManager().SetTimer(ParryTimerHandle, this, &ABossActionUECharacter::EndParry, WindowDuration, false);
	GetWorldTimerManager().SetTimer(ParryCooldownTimerHandle, this, &ABossActionUECharacter::FinishParryCooldown, CooldownDuration, false);

	UE_LOG(LogBossActionUE, Log, TEXT("Parry started"));
}

void ABossActionUECharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	GetWorldTimerManager().ClearTimer(ParryCooldownTimerHandle);

	bIsAttacking = false;
	bIsParrying = false;
	bParryOnCooldown = false;

	if (IsValid(HealthComponent))
	{
		HealthComponent->OnDeath.RemoveDynamic(this, &ABossActionUECharacter::HandleDeath);
	}

	Super::EndPlay(EndPlayReason);
}

void ABossActionUECharacter::EndParry()
{
	GetWorldTimerManager().ClearTimer(ParryTimerHandle);
	bIsParrying = false;

	UE_LOG(LogBossActionUE, Log, TEXT("Parry window ended"));
}

void ABossActionUECharacter::FinishParryCooldown()
{
	bParryOnCooldown = false;

	UE_LOG(LogBossActionUE, Log, TEXT("Parry ready"));
}
void ABossActionUECharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsRolling)
	{
		return;
	}

	// 지면을 벗어나면 구르기와 무적 종료
	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		FinishRoll();
		return;
	}

	// 구르기 방향으로만 이동 입력 전달
	ConsumeMovementInputVector();
	AddMovementInput(RollDirection, 1.f);
}

void ABossActionUECharacter::StartRoll()
{
	if (!IsValid(HealthComponent) || HealthComponent->IsDead())
	{
		return;
	}

	if (bIsAttacking || bIsParrying || bIsRolling || bRollOnCooldown)
	{
		UE_LOG(LogBossActionUE, Log, TEXT("Roll unavailable"));
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	if (!MovementComponent->IsMovingOnGround())
	{
		return;
	}

	// 최근 처리된 이동 입력 방향 사용
	RollDirection = GetLastMovementInputVector().GetSafeNormal2D();

	// 이동 입력이 없으면 캐릭터가 바라보는 방향 사용
	if (RollDirection.IsNearlyZero())
	{
		RollDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	SavedMaxWalkSpeed = MovementComponent->MaxWalkSpeed;

	const float ActualRollSpeed = FMath::Max(RollSpeed, 1.f);
	const float ActualRollDuration = FMath::Max(RollDuration, 0.01f);
	const float ActualRollCooldown = FMath::Max(RollCooldown, ActualRollDuration);

	bIsRolling = true;
	bRollOnCooldown = true;

	StopJumping();
	ConsumeMovementInputVector();
	MovementComponent->MaxWalkSpeed = ActualRollSpeed;

	// 출발할 때 즉시 구르기 속도 적용
	MovementComponent->Velocity.X = RollDirection.X * ActualRollSpeed;
	MovementComponent->Velocity.Y = RollDirection.Y * ActualRollSpeed;

	SetActorRotation(RollDirection.Rotation());

	GetWorldTimerManager().SetTimer(RollTimerHandle, this, &ABossActionUECharacter::FinishRoll, ActualRollDuration, false);
	GetWorldTimerManager().SetTimer(RollCooldownTimerHandle, this, &ABossActionUECharacter::FinishRollCooldown, ActualRollCooldown, false);

	UE_LOG(LogBossActionUE, Log, TEXT("Roll started"));
}

void ABossActionUECharacter::FinishRoll()
{
	GetWorldTimerManager().ClearTimer(RollTimerHandle);

	if (!bIsRolling)
	{
		return;
	}

	bIsRolling = false;

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

	MovementComponent->MaxWalkSpeed = SavedMaxWalkSpeed;

	// 수평 이동만 멈추고 낙하 속도는 유지
	MovementComponent->Velocity.X = 0.f;
	MovementComponent->Velocity.Y = 0.f;

	ConsumeMovementInputVector();

	UE_LOG(LogBossActionUE, Log, TEXT("Roll ended"));
}

void ABossActionUECharacter::FinishRollCooldown()
{
	bRollOnCooldown = false;

	UE_LOG(LogBossActionUE, Log, TEXT("Roll ready"));
}