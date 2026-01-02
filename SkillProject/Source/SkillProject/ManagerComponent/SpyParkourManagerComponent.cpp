// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    bIsWallClimbing = false;
    HitNormalVector = FVector::ZeroVector;

    VaultWallInfo = FVaultWallInfo();
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::CheckAbleWallClimbing()
{
    ASpyCharacter* SpyCharacter = Cast<ASpyCharacter>(GetOwner());
    if (SpyCharacter == nullptr)
        return;

    FVector Start = SpyCharacter->GetActorLocation() + SpyCharacter->GetActorForwardVector() * 20.f;
    FVector End = Start + SpyCharacter->GetActorForwardVector() * 40.f;

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(SpyCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

    FColor LineColor = bHit ? FColor::Green : FColor::Red;
    float Lifetime = 1.0f;
    float Thickness = 2.0f;

    DrawDebugLine(
        GetWorld(),
        Start,
        End,
        LineColor,
        false,
        Lifetime,
        0,
        Thickness
    );

    if (bIsWallClimbing != bHit)
    {
        bIsWallClimbing = bHit;
        HitNormalVector = Hit.ImpactNormal;

        ASpyPlayerController* SpyPlayerController = SpyCharacter->GetController<ASpyPlayerController>();
        ASpyPlayerState* SpyPlayerState = SpyCharacter->GetPlayerState<ASpyPlayerState>();

        if (SpyPlayerController == nullptr || SpyPlayerState == nullptr)
            return;

        if (bIsWallClimbing)
        {
            SpyPlayerState->AddState(ESpyPlayerStateFlags::IsClimb);
        }
        else
        {
            SpyPlayerState->RemoveState(ESpyPlayerStateFlags::IsClimb);
        }

        SpyPlayerController->SetMappingContext();
    }
}

bool USpyParkourManagerComponent::TryVaultAction()
{
    if (VaultMontage == nullptr)
        return false;

    ACharacter* OwnerChararacter = Cast<ACharacter>(GetOwner());
    if (OwnerChararacter == nullptr)
        return false;

    //# Vault 가능한 벽 정보 가져옴
    SetVaultWallInfo();

    //# 벽 정보가 세팅되지 않았다면 Vault 불가능한 벽
    if (VaultWallInfo.FrontNormalVector == FVector::ZeroVector &&
        VaultWallInfo.HandPosVector == FVector::ZeroVector &&
        VaultWallInfo.LandPosVector == FVector::ZeroVector)
        return false;

    UCharacterMovementComponent* CharacterMovementComponent = OwnerChararacter->GetCharacterMovement();
    UCapsuleComponent* CapsuleComponent = OwnerChararacter->GetCapsuleComponent();
    UAnimInstance* AnimInstance = OwnerChararacter->GetMesh()->GetAnimInstance();
    if (CharacterMovementComponent == nullptr || CapsuleComponent == nullptr || AnimInstance == nullptr)
        return false;

    //# Vault 벽 정보를 모션 워핑에 세팅
    SetMotionWarping();

    //# Vault Montage 시작 전 실행
    CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
    CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);

    //# Vault Montage 실행
    OwnerChararacter->PlayAnimMontage(VaultMontage);

    //# Vault Montage 끝난 후 실행
    FOnMontageEnded EndDelegate;
    EndDelegate.BindLambda([this, CharacterMovementComponent, CapsuleComponent](UAnimMontage* Montage, bool bInterrupted) {
        if (Montage == VaultMontage)
        {
            CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
            CharacterMovementComponent->StopMovementImmediately();
            CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
        }
        });

    AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultMontage);

    return true;
}

void USpyParkourManagerComponent::SetVaultWallInfo()
{
    VaultWallInfo.Clear();

    ACharacter* OwnerChararacter = Cast<ACharacter>(GetOwner());
    if (OwnerChararacter == nullptr)
        return;

    FVector OwnerLocation = OwnerChararacter->GetActorLocation();
    FVector OwnerFowardVector = OwnerChararacter->GetActorForwardVector();

    //# 재사용 변수들
    UWorld* World = GetWorld();
    FVector Start = OwnerLocation;
    FVector End = Start + (OwnerFowardVector * VaildDistance);
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChararacter);

    bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
    DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

    if (bHit)
    {
        FVector WallStart = HitResult.Location;
        FVector WallNormalVector = HitResult.ImpactNormal;

        VaultWallInfo.FrontNormalVector = WallNormalVector;
        VaultWallInfo.Distance = FVector::Distance(Start, HitResult.Location);
        
        float Interval = 50.f;
        FVector FlatLocation = HitResult.Location * FVector(1.f, 1.f, 0.f);

        bool CheckHeight = false;

        float MaxRayDistance = VaildDistance + VaildDepth;
        //# VaildHeight 만큼 충분히 높은 위치에서 Interval 간격으로 띄워서 반복 트레이스 검사
        for (int i = 1; i < 10; ++i)
        {
            //# 최대 거리를 넘었으면 두께 초과이니 레이로 확인할 필요가 없음
            if (MaxRayDistance < Interval * i)
            {
                VaultWallInfo.Clear();
                break;
            }

            //# 캐릭터 기준이 아닌 벽의 노말 벡터 기준으로 함
            FVector Next = -WallNormalVector * Interval * i;
            Start = FlatLocation + (FVector::UpVector * VaildHeight) + Next;
            End = Start + (FVector::DownVector * VaildHeight);
            HitResult.Reset();

            bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
            DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

            //# Hit 되면 벽의 높이 검사
            if (bHit)
            {
                if (CheckHeight == false)
                {
                    CheckHeight = true;
                    VaultWallInfo.HandPosVector = HitResult.Location;
                    VaultWallInfo.Height = FVector::Distance(HitResult.Location, End);
                }
            }
            //# Hit 되지 않은 트레이스에서 캐릭터 방향으로 역 트레이스 계산하여 벽의 두께 계산
            else
            {
                //# 다음 반복문 End 위치가 착지 지점
                VaultWallInfo.LandPosVector = Start + -WallNormalVector * Interval + (FVector::DownVector * VaildHeight);

                //# Z축 다시 캐릭터 Z축으로 
                End.Z = OwnerLocation.Z;
                
                Start = End;
                End = WallStart;
                HitResult.Reset();

                bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
                DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

                //# Hit 되면 벽의 두께 계산 후 반복문 종료
                if (bHit)
                {
                    VaultWallInfo.Depth = FVector::Distance(WallStart, HitResult.Location);
                    
                    //# Vault 가능한 두께가 아님
                    if (VaultWallInfo.Depth > VaildDepth)
                    {
                        VaultWallInfo.Clear();
                        return;
                    }
                }

                break;
            }
        }
    }
}

void USpyParkourManagerComponent::SetMotionWarping()
{
    ACharacter* OwnerChararacter = Cast<ACharacter>(GetOwner());
    if (OwnerChararacter == nullptr)
        return;

    UMotionWarpingComponent* MotionWarpingComponent = OwnerChararacter->FindComponentByClass<UMotionWarpingComponent>();
    if (MotionWarpingComponent != nullptr)
    {
        //# 벽 노말 벡터 기준으로 계산
        FRotator TargetRotator = VaultWallInfo.FrontNormalVector.GetSafeNormal2D().Rotation() - FRotator(0, 180.f, 0);
        FVector RightVector = FVector::CrossProduct(FVector::UpVector, -VaultWallInfo.FrontNormalVector);
        FVector ForwardVector = -VaultWallInfo.FrontNormalVector;
        FVector UpVector = FVector::UpVector;

        //# 애니메이션에 따라 오프셋 적용
        FVector FinalHandPos = VaultWallInfo.HandPosVector +
            (RightVector * VaultStartOffset.X) + //# X Offset
            (ForwardVector * VaultStartOffset.Y) + //# Y Offset
            (UpVector * VaultStartOffset.Z); //# Z Offset

        FVector FinalLandPos = VaultWallInfo.LandPosVector +
            (RightVector * VaultEndOffset.X) + //# X Offset
            (ForwardVector * VaultEndOffset.Y) + //# Y Offset
            (UpVector * VaultEndOffset.Z); //# Z Offset

        //# 디버그는 Offset 적용 안함
        DrawDebugSphere(GetWorld(), VaultWallInfo.HandPosVector, 10.f, 12, FColor::Yellow, false, 1.f);
        DrawDebugSphere(GetWorld(), VaultWallInfo.LandPosVector, 10.f, 12, FColor::Green, false, 1.f);

        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultStartName, FinalHandPos, TargetRotator);
        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultEndName, FinalLandPos, TargetRotator);
    }
}

