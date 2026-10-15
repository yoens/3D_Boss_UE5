#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossHealthComponent.generated.h"

// Delegate type for notifying listeners of death
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBossDeathSignature);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BOSSACTIONUE_API UBossHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBossHealthComponent();

	// Other objects can subscribe to this event
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FBossDeathSignature OnDeath;

	void ReceiveDamage(float DamageAmount);

	float GetCurrentHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	bool IsDead() const { return CurrentHealth <= 0.f; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health")
	float CurrentHealth = 0.f;
};