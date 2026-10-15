#include "BossEnemy.h"
#include "BossHealthComponent.h"
#include "BossActionUE.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
}

void ABossEnemy::BeginPlay()
{
    Super::BeginPlay();

    if (IsValid(HealthComponent))
    {
        HealthComponent->OnDeath.AddDynamic(this, &ABossEnemy::HandleDeath);
    }
}

void ABossEnemy::HandleDeath()
{
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();

    UE_LOG(LogBossActionUE, Log, TEXT("Enemy died: %s"), *GetName());

    Destroy();
}