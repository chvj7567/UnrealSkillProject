// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimInstance/SpyCharacterAnimInstance.h"
#include "ManagerComponent/SpyAnimManagerComponent.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "ManagerComponent/SpyParkourManagerComponent.h"
#include "Util/SpyGameplayTags.h"
#include "Manager/SpyAssetManager.h"
#include "Data/SpyAnimAssetData.h"
#include "Data/SpyAssetNames.h"
#include "Attribute/SKAttributeSet.h"
#include "ManagerComponent/SpyTargetingManagerComponent.h"
#include "System/SpyPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyCharacterAnimInstance)

void USpyCharacterAnimInstance::NativeBeginPlay()
{
    Super::NativeBeginPlay();

    if (USpyAnimAssetData* AnimData = USpyAssetManager::GetAssetByName<USpyAnimAssetData>(SpyAssetNames::AnimAssetData))
    {
        if (TSoftClassPtr<UAnimInstance> LayerSoftPtr = AnimData->AnimLayerMap.FindRef(SpyAnimLayerKeys::OneHandSword))
        {
            FSpyAssetAndDelegate LoadDelegate;
            LoadDelegate.BindLambda([this](UObject* LoadedAsset)
                {
                    if (UClass* LoadedClass = Cast<UClass>(LoadedAsset))
                    {
                        LinkAnimClassLayers(LoadedClass);
                    }
                });

            USpyAssetManager::LoadAssetAsync(LayerSoftPtr.ToSoftObjectPath(), LoadDelegate);
        }
    }

    APawn* PawnOwner = TryGetPawnOwner();
    if (PawnOwner == nullptr)
        return;

    Player = Cast<ASpyCharacter>(PawnOwner);
    if (Player == nullptr)
        return;

    PlayerMovementComponent = Player->GetSpyCharacterMovementComponent();
    if (PlayerMovementComponent == nullptr)
        return;
}

void USpyCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

    if (Player == nullptr || PlayerMovementComponent == nullptr)
        return;

    //# Set InputAngle
    {
        InputAngle = PlayerMovementComponent->GetInputAngleByForward();
    }

    //# Set AimPitch
    {
        FRotator Rot;
        if (Player->IsLocallyControlled())
        {
            //# 로컬 플레이어 -> 컨트롤러 기준
            Rot = Player->GetControlRotation();
        }
        else
        {
            //# 서버/원격 -> 캐릭터 기준
            Rot = Player->GetBaseAimRotation();
        }

        AimPitch = FRotator::NormalizeAxis(Rot.Pitch);
    }

    //# Set IsDeath And IsClimbing, Velocity, GroundSpeed, WallClimbSpeed
    {
        if (ASpyPlayerState* SpyPlayerState = Player->GetPlayerState<ASpyPlayerState>())
        {
            IsDeath = SpyPlayerState->GetAbilitySystemComponent()->HasMatchingGameplayTag(SKGameplayTags::Character_State_Death);
            IsClimbing = PlayerMovementComponent->IsClimbingActive();
            if (IsClimbing)
            {
                Velocity = PlayerMovementComponent->GetWallClimbSpeed();
                GroundSpeed = 0.0f;
                Speed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2) + FMath::Pow(Velocity.Z, 2));

                CurrentOffsetHL = PlayerMovementComponent->CurrentOffsetHL;
                CurrentOffsetHR = PlayerMovementComponent->CurrentOffsetHR;
                CurrentOffsetFL = PlayerMovementComponent->CurrentOffsetFL;
                CurrentOffsetFR = PlayerMovementComponent->CurrentOffsetFR;
            }
            else
            {
                Velocity = Player->GetVelocity();
                GroundSpeed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2));
                Speed = FMath::Sqrt(FMath::Pow(Velocity.X, 2) + FMath::Pow(Velocity.Y, 2) + FMath::Pow(Velocity.Z, 2));

                CurrentOffsetHL = FVector::Zero();
                CurrentOffsetHR = FVector::Zero();
                CurrentOffsetFL = FVector::Zero();
                CurrentOffsetFR = FVector::Zero();
            }
        }

        //#UE_LOG(LogTemp, Warning, TEXT("# CurrentOffsetHL X: %f Y: %f, Z: %f"), CurrentOffsetHL.X, CurrentOffsetHL.Y, CurrentOffsetHL.Z);
    }

    //# Set DirectionAngle (스트레이프 애니메이션용)
    {
        DirectionAngle = USpyAnimManagerComponent::CalcDirectionFromVelocity(
            Velocity, Player->GetActorRotation());
    }

    //# Set IsTargeting
    {
        if (USpyTargetingManagerComponent* TargetingComp = Player->FindComponentByClass<USpyTargetingManagerComponent>())
        {
            TWeakObjectPtr<AActor> Target = TargetingComp->GetTarget();
            if (Target.IsValid())
            {
                IsTargeting = true;
            }
            else
            {
                IsTargeting = false;
            }
        }
    }

    //# Set ShouldMove
    {
        FVector CurrentAcceleration = PlayerMovementComponent->GetCurrentAcceleration();
        if (CurrentAcceleration.IsZero() == false && GroundSpeed > 3.f)
        {
            ShouldMove = true;
        }
        else
        {
            ShouldMove = false;
        }
    }

    //# Set IsFalling And IsCrouching
    {
        IsFalling = PlayerMovementComponent->IsFalling();
        IsCrouching = PlayerMovementComponent->IsCrouching();
    }
}

void USpyCharacterAnimInstance::AnimNotify_AttackHit(UAnimNotify* Notify)
{
    //#Player->TestHit();
}