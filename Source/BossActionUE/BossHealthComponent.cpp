#include "BossHealthComponent.h"
#include "GameFramework/Actor.h"

UBossHealthComponent::UBossHealthComponent()
{
	// Health does not need a per-frame update
	PrimaryComponentTick.bCanEverTick = false;
}

void UBossHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(MaxHealth, 1.f);
	CurrentHealth = MaxHealth;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s initialized HP: %.0f / %.0f"),
		*GetNameSafe(GetOwner()),
		CurrentHealth,
		MaxHealth
	);
}

void UBossHealthComponent::ReceiveDamage(float DamageAmount)
{
	// Ignore non-positive damage and damage after death
	if (DamageAmount <= 0.f || IsDead())
	{
		return;
	}

	CurrentHealth = FMath::Clamp(
		CurrentHealth - DamageAmount,
		0.f,
		MaxHealth
	);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s HP: %.0f / %.0f"),
		*GetNameSafe(GetOwner()),
		CurrentHealth,
		MaxHealth
	);

	if (IsDead())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s died"),
			*GetNameSafe(GetOwner())
		);
		OnDeath.Broadcast();
	}
}