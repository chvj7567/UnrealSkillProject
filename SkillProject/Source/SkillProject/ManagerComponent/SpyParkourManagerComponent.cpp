// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/DefineEnum.h"
#include "GameFramework/Character.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    ClimbWallData = FClimbWallData();
    VaultWallData = FVaultWallData();
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::TryClimbAction()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return;

    ASpyPlayerController* SpyPlayerController = OwnerCharacter->GetController<ASpyPlayerController>();
    ASpyPlayerState* SpyPlayerState = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    if (SpyPlayerController == nullptr || SpyPlayerState == nullptr)
        return;

    if (SpyPlayerState->HasState(ESpyPlayerStateFlags::IsClimb))
    {
        SpyPlayerState->RemoveState(ESpyPlayerStateFlags::IsClimb);
    }
    else
    {
        FVector OwnerLocation = OwnerCharacter->GetActorLocation();
        FVector OwnerFowardVector = OwnerCharacter->GetActorForwardVector();

        FVector Start = OwnerLocation;
        FVector End = Start + (OwnerFowardVector * ClimbData.DistanceOffset);

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(OwnerCharacter);

        bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_WorldStatic, Params);
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, 2.f);

        ClimbWallData.HitVector = Hit.Location;
        ClimbWallData.NormalVector = Hit.ImpactNormal;

        if (bHit)
        {
            SpyPlayerState->AddState(ESpyPlayerStateFlags::IsClimb);
        }
    }

    SpyPlayerController->RefreshMappingContext();
}

bool USpyParkourManagerComponent::TryVaultAction()
{
    if (VaultData.VaultMontage == nullptr)
        return false;

    ACharacter* OwnerChararacter = Cast<ACharacter>(GetOwner());
    if (OwnerChararacter == nullptr)
        return false;

    //# Vault 가능한 벽 정보 가져옴
    SetVaultWallInfo();

    //# 벽 정보가 세팅되지 않았다면 Vault 불가능한 벽
    if (VaultWallData.FrontNormalVector == FVector::ZeroVector &&
        VaultWallData.HandPosVector == FVector::ZeroVector &&
        VaultWallData.LandPosVector == FVector::ZeroVector)
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
    OwnerChararacter->PlayAnimMontage(VaultData.VaultMontage);

    //# Vault Montage 끝난 후 실행
    FOnMontageEnded EndDelegate;
    EndDelegate.BindLambda([this, CharacterMovementComponent, CapsuleComponent](UAnimMontage* Montage, bool bInterrupted) {
        if (Montage == VaultData.VaultMontage)
        {
            UE_LOG(LogTemp, Warning, TEXT("Montage End"));
            CharacterMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
            CharacterMovementComponent->StopMovementImmediately();
            CapsuleComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
        }
        });

    AnimInstance->Montage_SetEndDelegate(EndDelegate, VaultData.VaultMontage);

    return true;
}

void USpyParkourManagerComponent::SetVaultWallInfo()
{
    VaultWallData.Clear();

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return;

    FVector OwnerLocation = OwnerCharacter->GetActorLocation();
    FVector OwnerFowardVector = OwnerCharacter->GetActorForwardVector();

    //# 재사용 변수들
    UWorld* World = GetWorld();
    FVector Start = OwnerLocation;
    FVector End = Start + (OwnerFowardVector * VaultData.VaildDistance);
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
    DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

    if (bHit)
    {
        FVector WallStart = HitResult.Location;
        FVector WallNormalVector = HitResult.ImpactNormal;

        VaultWallData.FrontNormalVector = WallNormalVector;
        VaultWallData.Distance = FVector::Distance(Start, HitResult.Location);
        
        FVector FlatLocation = HitResult.Location * FVector(1.f, 1.f, 0.f);

        bool CheckHeight = false;

        float MaxRayDistance = VaultData.VaildDistance + VaultData.VaildDepth;
        //# VaildHeight 만큼 충분히 높은 위치에서 RayInterval 간격으로 띄워서 반복 트레이스 검사
        for (int i = 1; i <= VaultData.RayIntervalReapeatCount; ++i)
        {
            //# 최대 거리를 넘었으면 두께 초과이니 레이로 확인할 필요가 없음
            if (MaxRayDistance < VaultData.RayInterval * i)
            {
                VaultWallData.Clear();
                break;
            }

            //# 캐릭터 기준이 아닌 벽의 노말 벡터 기준으로 함
            FVector Next = -WallNormalVector * VaultData.RayInterval * i;
            Start = FlatLocation + (FVector::UpVector * VaultData.VaildHeight) + Next;
            End = Start + (FVector::DownVector * VaultData.VaildHeight);
            HitResult.Reset();

            bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
            DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

            //# Hit 되면 벽의 높이 검사
            if (bHit)
            {
                if (CheckHeight == false)
                {
                    CheckHeight = true;
                    VaultWallData.HandPosVector = HitResult.Location;
                    VaultWallData.Height = FVector::Distance(HitResult.Location, End);
                }
            }
            //# Hit 되지 않은 트레이스에서 캐릭터 방향으로 역 트레이스 계산하여 벽의 두께 계산
            else
            {
                //# 다음 반복문 End 위치가 착지 지점
                VaultWallData.LandPosVector = Start + -WallNormalVector * VaultData.RayInterval + (FVector::DownVector * VaultData.VaildHeight);

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
                    VaultWallData.Depth = FVector::Distance(WallStart, HitResult.Location);
                    
                    //# Vault 가능한 두께가 아님
                    if (VaultWallData.Depth > VaultData.VaildDepth)
                    {
                        VaultWallData.Clear();
                    }

                    return;
                }

                break;
            }
        }

        //# 벽이 지정한 레이 간격의 반복 횟수보다 더 두꺼움
        VaultWallData.Clear();
        return;
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
        FRotator TargetRotator = VaultWallData.FrontNormalVector.GetSafeNormal2D().Rotation() - FRotator(0, 180.f, 0);
        FVector RightVector = FVector::CrossProduct(FVector::UpVector, -VaultWallData.FrontNormalVector);
        FVector ForwardVector = -VaultWallData.FrontNormalVector;
        FVector UpVector = FVector::UpVector;

        //# 애니메이션에 따라 오프셋 적용
        FVector FinalHandPos = VaultWallData.HandPosVector +
            (RightVector * VaultData.VaultStartOffset.X) + //# X Offset
            (ForwardVector * VaultData.VaultStartOffset.Y) + //# Y Offset
            (UpVector * VaultData.VaultStartOffset.Z); //# Z Offset

        FVector FinalLandPos = VaultWallData.LandPosVector +
            (RightVector * VaultData.VaultEndOffset.X) + //# X Offset
            (ForwardVector * VaultData.VaultEndOffset.Y) + //# Y Offset
            (UpVector * VaultData.VaultEndOffset.Z); //# Z Offset

        //# 디버그는 Offset 적용 안함
        DrawDebugSphere(GetWorld(), VaultWallData.HandPosVector, 10.f, 12, FColor::Yellow, false, 1.f);
        DrawDebugSphere(GetWorld(), VaultWallData.LandPosVector, 10.f, 12, FColor::Green, false, 1.f);

        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultData.VaultStartName, FinalHandPos, TargetRotator);
        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultData.VaultEndName, FinalLandPos, TargetRotator);
    }
}