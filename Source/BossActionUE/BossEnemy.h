#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "BossEnemy.generated.h"

class UBossHealthComponent;
class UBossHealthComponent;
class ABossActionUECharacter;

UCLASS()
class BOSSACTIONUE_API ABossEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ABossEnemy();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 적의 체력 관리 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBossHealthComponent* HealthComponent;

    /** 플레이어를 감지하는 거리 */
    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.0"))
    float DetectRange = 800.f;

    /** 추적 이동을 완료했다고 판단하는 허용 반경 */
    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (ClampMin = "0.0"))
    float ChaseAcceptanceRadius = 100.f;

    /** 공격을 시작하는 거리 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1.0"))
    float AttackRange = 220.f;

    /** 타격 순간에 검사하는 최대 거리 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "1.0"))
    float AttackHitRange = 240.f;

    /** 한 번의 공격 피해량 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0"))
    float AttackDamage = 10.f;

    /** 공격 준비 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.01"))
    float AttackWindup = 0.6f;

    /** 타격 이후 행동 제한 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.01"))
    float AttackRecovery = 0.5f;

    /** 후딜레이 종료 후 재사용 대기 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.01"))
    float AttackCooldown = 1.5f;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat")
    bool bIsAttacking = false;

    UPROPERTY(VisibleInstanceOnly, Category = "Combat")
    bool bAttackOnCooldown = false;

    /** 패링당했을 때 적이 받는 피해 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Parry", meta = (ClampMin = "0.0"))
    float ParryDamage = 10.f;

    /** 패링당했을 때 경직 시간 */
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Parry", meta = (ClampMin = "0.01"))
    float ParryStunDuration = 1.f;

    /** 현재 경직 중인지 */
    UPROPERTY(VisibleInstanceOnly, Category = "Combat|Parry")
    bool bIsStunned = false;

    /** 공격 시작 시 선택한 대상 */
    TWeakObjectPtr<ABossActionUECharacter> AttackTarget;
    /** 추적 여부를 판단하는 타이머 */
    FTimerHandle AITimerHandle;
    FTimerHandle AttackTimerHandle;
    FTimerHandle AttackCooldownTimerHandle;
    FTimerHandle StunTimerHandle;

    void HandleParried();
    void FinishStun();
    void StartAttack(ABossActionUECharacter* TargetPlayer);
    void ExecuteAttack();
    void FinishAttack();
    void FinishAttackCooldown();
    void UpdateAI();

    UFUNCTION()
    void HandleDeath();
};