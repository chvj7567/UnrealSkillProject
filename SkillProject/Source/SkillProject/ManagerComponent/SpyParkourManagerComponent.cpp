// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/SpyGameplayTags.h"
#include "GameFramework/Character.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"
#include "Character/SpyCharacter.h"
#include "MotionWarpingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyParkourManagerComponent)

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);
}

void USpyParkourManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(USpyParkourManagerComponent, bFreeMoveMode);
    DOREPLIFETIME(USpyParkourManagerComponent, ClimbWallData);
    DOREPLIFETIME(USpyParkourManagerComponent, VaultMotionWarpingData);
    DOREPLIFETIME(USpyParkourManagerComponent, HangUpMotionWarpingData);
}

void USpyParkourManagerComponent::OnRep_FreeMoveMode()
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement())
        {
            if (bFreeMoveMode)
            {
                CMC->SetMovementMode(EMovementMode::MOVE_Flying);
            }
            else
            {
                CMC->SetMovementMode(EMovementMode::MOVE_Walking);
            }
        }

        if (UCapsuleComponent* CC = OwnerCharacter->GetCapsuleComponent())
        {
            if (bFreeMoveMode)
            {
                CC->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
            }
            else
            {
                CC->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
            }
        }
    }
}

bool USpyParkourManagerComponent::TryToggleClimbAction()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return false;

    ASpyPlayerState* SpyPlayerState = OwnerCharacter->GetPlayerState<ASpyPlayerState>();
    if (SpyPlayerState == nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("# [Parkour] SpyPlayerState Is Null"));
        return false;
    }

    ASpyPlayerController* SpyPlayerController = OwnerCharacter->GetController<ASpyPlayerController>();
    if (SpyPlayerController == nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("# [Parkour] SpyPlayerController Is Null"));
        return false;
    }

    FVector OwnerLocation = OwnerCharacter->GetActorLocation();
    FVector OwnerFowardVector = OwnerCharacter->GetActorForwardVector();

    FVector Start = OwnerLocation;
    FVector End = Start + (OwnerFowardVector * ClimbData.DistanceOffset);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_WorldStatic, Params);
    DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, 2.f);

    if (bHit)
    {
        //# 서버만 실행
        if (OwnerCharacter->HasAuthority())
        {
            FClimbWallData newWallData = FClimbWallData();
            newWallData.HitVector = Hit.ImpactPoint;
            newWallData.NormalVector = Hit.ImpactNormal;

            ClimbWallData = newWallData;
            OnRep_ClimbWallData();
        }
    }

    return bHit;
}

void USpyParkourManagerComponent::OnRep_ClimbWallData()
{
    if (OnClimbData.IsBound())
    {
        OnClimbData.Broadcast(ClimbData, ClimbWallData);
    }
}

bool USpyParkourManagerComponent::CanVaultAction()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return false;

    //# Vault 가능한 벽 정보 가져옴
    SetVaultWallData();

    //# 벽 정보가 세팅되지 않았다면 Vault 불가능한 벽
    if (VaultWallData.FrontNormalVector == FVector::ZeroVector &&
        VaultWallData.HandLocVector == FVector::ZeroVector &&
        VaultWallData.LandLocVector == FVector::ZeroVector)
        return false;

    return true;
}

void USpyParkourManagerComponent::OnRep_VaultMotionWarpingData()
{
    if (OnVaultMotionWarpingData.IsBound())
    {
        OnVaultMotionWarpingData.Broadcast(VaultMotionWarpingData);
    }
}

void USpyParkourManagerComponent::OnRep_HangUpMotionWarpingData()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return;

    if (OwnerCharacter->GetLocalRole() == ROLE_Authority)
        UE_LOG(LogTemp, Warning, TEXT("##2 ROLE_Authority"));
    if (OwnerCharacter->GetLocalRole() == ROLE_AutonomousProxy)
        UE_LOG(LogTemp, Warning, TEXT("##2 ROLE_AutonomousProxy"));

    if (OnHangUpMotionWarpingData.IsBound())
    {
        OnHangUpMotionWarpingData.Broadcast(HangUpMotionWarpingData);
    }
}

void USpyParkourManagerComponent::SetVaultWallData()
{
    VaultWallData.Clear();

    if (SetValidWallData(
        VaultData.VaildDistance,
        VaultData.VaildHeight,
        VaultData.VaildDepth,
        VaultData.RayInterval,
        VaultData.RayIntervalReapeatCount) == false)
        return;

    VaultWallData.FrontNormalVector = WallData.NormalVector;
    VaultWallData.HandLocVector = WallData.HitVector;
    VaultWallData.LandLocVector = WallData.LandVector;
    VaultWallData.Distance = WallData.Distance;
    VaultWallData.Height = WallData.Height;
    VaultWallData.Depth = WallData.Depth;
}

void USpyParkourManagerComponent::SetVaultMotionWarpingData()
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return;

    //# 벽 노말 벡터 기준으로 계산
    FRotator TargetRotator = VaultWallData.FrontNormalVector.GetSafeNormal2D().Rotation() - FRotator(0, 180.f, 0);
    FVector RightVector = FVector::CrossProduct(FVector::UpVector, -VaultWallData.FrontNormalVector);
    FVector ForwardVector = -VaultWallData.FrontNormalVector;
    FVector UpVector = FVector::UpVector;

    //# 애니메이션에 따라 오프셋 적용
    FVector FinalHandLoc = VaultWallData.HandLocVector +
        (RightVector * VaultData.VaultStartOffset.X) + //# X Offset
        (ForwardVector * VaultData.VaultStartOffset.Y) + //# Y Offset
        (UpVector * VaultData.VaultStartOffset.Z); //# Z Offset

    FVector FinalLandLoc = VaultWallData.LandLocVector +
        (RightVector * VaultData.VaultEndOffset.X) + //# X Offset
        (ForwardVector * VaultData.VaultEndOffset.Y) + //# Y Offset
        (UpVector * VaultData.VaultEndOffset.Z); //# Z Offset

    //# 디버그는 Offset 적용 안함
    DrawDebugSphere(GetWorld(), VaultWallData.HandLocVector, 10.f, 12, FColor::Yellow, false, 1.f);
    DrawDebugSphere(GetWorld(), VaultWallData.LandLocVector, 10.f, 12, FColor::Green, false, 1.f);

    if (OwnerCharacter->HasAuthority())
    {
        FMotionWarpingData Data;
        Data.StartLoc = FinalHandLoc;
        Data.StartRot = TargetRotator;
        Data.EndLoc = FinalLandLoc;
        Data.EndRot = TargetRotator;

        VaultMotionWarpingData = Data;

        OnRep_VaultMotionWarpingData();
    }
}

void USpyParkourManagerComponent::SetHangUpMotionWarpingData(const FVector& HitVector)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return;

    //# 벽 노말 벡터 기준으로 계산
    FRotator TargetRotator = OwnerCharacter->GetActorRotation();
    FVector RightVector = OwnerCharacter->GetActorRightVector();
    FVector ForwardVector = OwnerCharacter->GetActorForwardVector();
    FVector UpVector = FVector::UpVector;

    //# 애니메이션에 따라 오프셋 적용
    FVector FinalHandLoc = HitVector +
        (RightVector * HangUpData.HangUpStartOffset.X) + //# X Offset
        (ForwardVector * HangUpData.HangUpStartOffset.Y) + //# Y Offset
        (UpVector * HangUpData.HangUpStartOffset.Z); //# Z Offset

    FVector FinalLandLoc = HitVector +
        (RightVector * HangUpData.HangUpEndOffset.X) + //# X Offset
        (ForwardVector * HangUpData.HangUpEndOffset.Y) + //# Y Offset
        (UpVector * HangUpData.HangUpEndOffset.Z); //# Z Offset

    //# 디버그는 Offset 적용 안함
    DrawDebugSphere(GetWorld(), HitVector, 10.f, 12, FColor::Blue, false, 1.f);

    if (OwnerCharacter->HasAuthority())
    {
        FMotionWarpingData Data;
        Data.StartLoc = FinalHandLoc;
        Data.StartRot = TargetRotator;
        Data.EndLoc = FinalLandLoc;
        Data.EndRot = TargetRotator;

        HangUpMotionWarpingData = Data;

        OnRep_HangUpMotionWarpingData();
    }
}

bool USpyParkourManagerComponent::SetValidWallData(float InValidDistance, float InValidHeight, float InValidDepth, int InRayInterval, int InRayIntervalReapeatCount)
{
    WallData.Clear();

    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter == nullptr)
        return false;

    FVector OwnerLocation = OwnerCharacter->GetActorLocation();
    FVector OwnerFowardVector = OwnerCharacter->GetActorForwardVector();

    //# 재사용 변수들
    UWorld* World = GetWorld();
    FVector Start = OwnerLocation;
    FVector End = Start + (OwnerFowardVector * InValidDistance);
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerCharacter);

    bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
    DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

    if (bHit)
    {
        FVector WallStart = HitResult.Location;
        FVector WallNormalVector = HitResult.ImpactNormal;

        WallData.NormalVector = WallNormalVector;
        WallData.Distance = FVector::Distance(Start, HitResult.Location);

        FVector FlatLocation = HitResult.Location * FVector(1.f, 1.f, 0.f);

        bool CheckHeight = false;
        float HeightOffset = 1000.f;

        float MaxRayDistance = InValidDistance + InValidDepth;
        //# VaildHeight 만큼 충분히 높은 위치에서 RayInterval 간격으로 띄워서 반복 트레이스 검사
        for (int i = 1; i <= InRayIntervalReapeatCount; ++i)
        {
            //# 최대 거리를 넘었으면 두께 초과이니 레이로 확인할 필요가 없음
            if (MaxRayDistance < InRayInterval * i)
            {
                WallData.Clear();
                return false;
            }

            //# 캐릭터 기준이 아닌 벽의 노말 벡터 기준으로 함
            FVector Next = -WallNormalVector * InRayInterval * i;
            
            //# 충분히 높은 곳에서 레이 발사하여 높이 확인
            Start = FlatLocation + (FVector::UpVector * (InValidHeight + HeightOffset)) + Next;
            End = Start + (FVector::DownVector * (InValidHeight + HeightOffset));
            HitResult.Reset();

            bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
            DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

            //# Hit 되면 벽의 높이 검사
            if (bHit)
            {
                if (CheckHeight == false)
                {
                    CheckHeight = true;
                    WallData.HitVector = HitResult.Location;
                    WallData.Height = FVector::Distance(HitResult.Location, End);

                    //# 높이 유효성 검사
                    if (WallData.Height > InValidHeight)
                    {
                        WallData.Clear();
                        return false;
                    }
                }
            }
            //# Hit 되지 않은 트레이스에서 캐릭터 방향으로 역 트레이스 계산하여 벽의 두께 계산
            else
            {
                //# 다음 반복문 End 위치가 착지 지점
                WallData.LandVector = Start + -WallNormalVector * VaultData.RayInterval + (FVector::DownVector * (VaultData.VaildHeight + HeightOffset));

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
                    WallData.Depth = FVector::Distance(WallStart, HitResult.Location);

                    //# Vault 가능한 두께가 아님
                    if (WallData.Depth > InValidDepth)
                    {
                        WallData.Clear();
                        return false;
                    }

                    return true;
                }

                break;
            }
        }

        //# 벽이 지정한 레이 간격의 반복 횟수보다 더 두꺼움
        WallData.Clear();
        return false;
    }

    return false;
}
