#include "BossEnemy.h"
#include "BossHealthComponent.h"
#include "BossActionUE.h"
#include "BossActionUECharacter.h"
#include "AIController.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

ABossEnemy::ABossEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
    GetCharacterMovement()->MaxWalkSpeed = 300.f;

    HealthComponent = CreateDefaultSubobject<UBossHealthComponent>(TEXT("HealthComponent"));

    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ABossEnemy::BeginPlay()
{
    Super::BeginPlay();

    if (IsValid(HealthComponent))
    {
        HealthComponent->OnDeath.AddDynamic(this, &ABossEnemy::HandleDeath);
    }

    GetWorldTimerManager().SetTimer(AITimerHandle, this, &ABossEnemy::UpdateAI, 0.2f, true);
}

void ABossEnemy::UpdateAI()
{
    AAIController* EnemyController = Cast<AAIController>(GetController());

    if (!IsValid(EnemyController))
    {
        return;
    }

    if (!IsValid(HealthComponent) || HealthComponent->IsDead())
    {
        EnemyController->StopMovement();
        return;
    }

    // 공격 준비와 후딜레이 동안 추적하지 않음
    if (bIsAttacking || bIsStunned)
    {
        return;
    }

    ABossActionUECharacter* TargetPlayer = Cast<ABossActionUECharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

    if (!IsValid(TargetPlayer))
    {
        EnemyController->StopMovement();
        return;
    }

    UBossHealthComponent* TargetHealthComponent = TargetPlayer->FindComponentByClass<UBossHealthComponent>();

    if (!IsValid(TargetHealthComponent) || TargetHealthComponent->IsDead())
    {
        EnemyController->StopMovement();
        return;
    }

    const float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

    if (DistanceToPlayer > DetectRange)
    {
        EnemyController->StopMovement();
        return;
    }

    if (DistanceToPlayer <= AttackRange)
    {
        EnemyController->StopMovement();

        if (!bAttackOnCooldown)
        {
            StartAttack(TargetPlayer);
        }

        return;
    }

    if (EnemyController->GetMoveStatus() == EPathFollowingStatus::Moving)
    {
        return;
    }

    EnemyController->MoveToActor(TargetPlayer, ChaseAcceptanceRadius, true);
}

void ABossEnemy::HandleDeath()
{
    GetWorldTimerManager().ClearTimer(AITimerHandle);
    GetWorldTimerManager().ClearTimer(AttackTimerHandle);
    GetWorldTimerManager().ClearTimer(AttackCooldownTimerHandle);
    GetWorldTimerManager().ClearTimer(StunTimerHandle);

    bIsStunned = false;
    bIsAttacking = false;
    bAttackOnCooldown = false;
    AttackTarget.Reset();

    AAIController* EnemyController = Cast<AAIController>(GetController());

    if (IsValid(EnemyController))
    {
        EnemyController->StopMovement();
    }

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy died: %s"), *GetName());

    Destroy();
}

void ABossEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(AITimerHandle);
    GetWorldTimerManager().ClearTimer(AttackTimerHandle);
    GetWorldTimerManager().ClearTimer(AttackCooldownTimerHandle);
    GetWorldTimerManager().ClearTimer(StunTimerHandle);

    Super::EndPlay(EndPlayReason);
}
void ABossEnemy::StartAttack(ABossActionUECharacter* TargetPlayer)
{
    if (!IsValid(TargetPlayer) || bIsAttacking || bAttackOnCooldown || bIsStunned)
    {
        return;
    }

    bIsAttacking = true;
    AttackTarget = TargetPlayer;

    GetCharacterMovement()->StopMovementImmediately();

    // 공격 시작 시 플레이어 방향으로 회전
    const FVector AttackDirection = (TargetPlayer->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

    if (!AttackDirection.IsNearlyZero())
    {
        SetActorRotation(AttackDirection.Rotation());
    }

    const float WindupDuration = FMath::Max(AttackWindup, 0.01f);
    const FVector ArrowEnd = GetActorLocation() + GetActorForwardVector() * AttackHitRange;

    // 노란 화살표: 공격 준비 방향
    DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), ArrowEnd, 30.f, FColor::Yellow, false, WindupDuration, 0, 3.f);

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy attack windup"));

    GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ABossEnemy::ExecuteAttack, WindupDuration, false);
}

void ABossEnemy::ExecuteAttack()
{
    if (!bIsAttacking || !IsValid(HealthComponent) || HealthComponent->IsDead())
    {
        return;
    }

    // 적중 여부와 관계없이 후딜레이 적용
    GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ABossEnemy::FinishAttack, FMath::Max(AttackRecovery, 0.01f), false);

    const FVector ArrowEnd = GetActorLocation() + GetActorForwardVector() * AttackHitRange;

    // 빨간 화살표: 타격 시점
    DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), ArrowEnd, 30.f, FColor::Red, false, 0.2f, 0, 3.f);

    ABossActionUECharacter* TargetPlayer = AttackTarget.Get();

    if (!IsValid(TargetPlayer))
    {
        return;
    }

    const FVector ToTarget = TargetPlayer->GetActorLocation() - GetActorLocation();

    // 타격 순간에 범위를 벗어나면 빗나감
    if (ToTarget.Size() > AttackHitRange)
    {
        UE_LOG(LogBossActionUE, Log, TEXT("Enemy attack missed: out of range"));
        return;
    }

    // 정면 기준 좌우 60도 이내인지 검사
    const FVector TargetDirection = ToTarget.GetSafeNormal2D();
    const float ForwardDot = FVector::DotProduct(GetActorForwardVector(), TargetDirection);

    if (ForwardDot < 0.5f)
    {
        UE_LOG(LogBossActionUE, Log, TEXT("Enemy attack missed: outside attack angle"));
        return;
    }

    // 적과 플레이어 사이에 벽 등이 있는지 검사
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(TargetPlayer);

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(HitResult, GetActorLocation(), TargetPlayer->GetActorLocation(), ECC_Visibility, QueryParams);

    if (bBlocked)
    {
        UE_LOG(LogBossActionUE, Log, TEXT("Enemy attack blocked by obstacle"));
        return;
    }

    const EPlayerHitResult HitResultType = TargetPlayer->ReceiveHit(AttackDamage);

    if (HitResultType == EPlayerHitResult::Parried)
    {
        HandleParried();
    }
}

void ABossEnemy::FinishAttack()
{
    bIsAttacking = false;
    bAttackOnCooldown = true;
    AttackTarget.Reset();

    GetWorldTimerManager().SetTimer(AttackCooldownTimerHandle, this, &ABossEnemy::FinishAttackCooldown, FMath::Max(AttackCooldown, 0.01f), false);

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy attack finished"));
}

void ABossEnemy::FinishAttackCooldown()
{
    bAttackOnCooldown = false;
}
void ABossEnemy::HandleParried()
{
    if (!IsValid(HealthComponent) || HealthComponent->IsDead())
    {
        return;
    }

    // 예약된 공격 후딜레이와 쿨타임 취소
    GetWorldTimerManager().ClearTimer(AttackTimerHandle);
    GetWorldTimerManager().ClearTimer(AttackCooldownTimerHandle);

    bIsAttacking = false;
    bAttackOnCooldown = true;
    bIsStunned = true;
    AttackTarget.Reset();

    AAIController* EnemyController = Cast<AAIController>(GetController());

    if (IsValid(EnemyController))
    {
        EnemyController->StopMovement();
    }

    GetCharacterMovement()->StopMovementImmediately();

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy parried: stunned"));

    HealthComponent->ReceiveDamage(ParryDamage);

    // 패링 피해로 죽었다면 경직 타이머를 만들지 않음
    if (HealthComponent->IsDead())
    {
        return;
    }

    GetWorldTimerManager().SetTimer(StunTimerHandle, this, &ABossEnemy::FinishStun, FMath::Max(ParryStunDuration, 0.01f), false);
}

void ABossEnemy::FinishStun()
{
    bIsStunned = false;

    // 경직 종료 후 공격 쿨타임 시작
    GetWorldTimerManager().SetTimer(AttackCooldownTimerHandle, this, &ABossEnemy::FinishAttackCooldown, FMath::Max(AttackCooldown, 0.01f), false);

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy stun ended"));
}