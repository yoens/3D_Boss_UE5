#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossEnemy.generated.h"

class UBossHealthComponent;

UCLASS()
class BOSSACTIONUE_API ABossEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ABossEnemy();

protected:
    virtual void BeginPlay() override;

    /** 적의 체력 관리 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBossHealthComponent* HealthComponent;

    /** 체력이 0이 되면 호출 */
    UFUNCTION()
    void HandleDeath();
};