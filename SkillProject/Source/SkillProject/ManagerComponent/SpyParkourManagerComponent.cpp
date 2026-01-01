// Fill out your copyright notice in the Description page of Project Settings.


#include "SpyParkourManagerComponent.h"
#include "Util/DefineEnum.h"
#include "Character/SpyCharacter.h"
#include "Character/SpyCharacterMovementComponent.h"
#include "System/SpyPlayerController.h"
#include "System/SpyPlayerState.h"
#include "MotionWarpingComponent.h"

USpyParkourManagerComponent::USpyParkourManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

    bIsWallClimbing = false;
    HitNormalVector = FVector::ZeroVector;
}

void USpyParkourManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpyParkourManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //# 벽 체크
    //CheckAbleWallClimbing();
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

FVaultWallInfo USpyParkourManagerComponent::GetVaultWallInfo()
{
    ACharacter* OwnerChararacter = Cast<ACharacter>(GetOwner());
    if (OwnerChararacter == nullptr)
        return FVaultWallInfo();

    VaultWallInfo.Clear();

    //# 임시 저장 변수들
    FVector OwnerLocation = OwnerChararacter->GetActorLocation();
    FVector OwnerFowardVector = OwnerChararacter->GetActorForwardVector();
    FVector HandPos;
    FVector LandPos;
    FVector WallStart;
    FVector WallNormalVector;

    //# 재사용 변수들
    UWorld* World = GetWorld();
    FVector Start = OwnerLocation;
    FVector End = Start + (OwnerFowardVector * 100.f/*VaildDistance*/);
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChararacter);

    bool bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
    DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

    if (bHit)
    {
        WallStart = HitResult.Location;
        WallNormalVector = HitResult.ImpactNormal;
        VaultWallInfo.Distance = FVector::Distance(Start, HitResult.Location);
        
        float SaftyOffset = 300.f;
        float Interval = 50.f;
        FVector FlatLocation = HitResult.Location * FVector(1.f, 1.f, 0.f);

        bool CheckHeight = false;
        //# SaftyOffset 만큼 충분히 높은 위치에서 Interval 간격으로 띄워서 반복 트레이스 검사
        for (int i = 1; i < 10; ++i)
        {
            //# 캐릭터 기준이 아닌 벽의 노말 벡터 기준으로 함
            FVector Next = -WallNormalVector * Interval * i;
            Start = FlatLocation + (FVector::UpVector * SaftyOffset) + Next;
            End = Start + (FVector::DownVector * SaftyOffset);
            HitResult.Reset();

            bHit = World->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, Params);
            DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 1.f, 0, -1.f);

            //# Hit 되면 벽의 높이 검사
            if (bHit)
            {
                if (CheckHeight == false)
                {
                    CheckHeight = true;
                    HandPos = HitResult.Location;
                    VaultWallInfo.Height = FVector::Distance(HitResult.Location, End);
                }
            }
            //# Hit 되지 않은 트레이스에서 캐릭터 방향으로 역 트레이스 계산하여 벽의 두께 계산
            else
            {
                //# 다음 반복문 End 위치가 착지 지점
                LandPos = Start + -WallNormalVector * Interval + (FVector::DownVector * SaftyOffset);

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

                    UE_LOG(LogTemp, Warning, TEXT("Distance %f, Height %f, Depth %f"), VaultWallInfo.Distance, VaultWallInfo.Height, VaultWallInfo.Depth);
                    
                    UMotionWarpingComponent* MotionWarpingComponent = OwnerChararacter->FindComponentByClass<UMotionWarpingComponent>();
                    if (MotionWarpingComponent != nullptr)
                    {
                        //# 벽 노말 벡터 기준으로 계산
                        FRotator TargetRotator = WallNormalVector.GetSafeNormal2D().Rotation() - FRotator(0, 180.f, 0);
                        FVector RightVector = FVector::CrossProduct(FVector::UpVector, -WallNormalVector);
                        FVector ForwardVector = -WallNormalVector;
                        FVector UpVector = FVector::UpVector;

                        //# 애니메이션에 따라 오프셋 적용
                        FVector FinalHandPos = HandPos +
                            (RightVector * VaultStartOffset.X) + //# X Offset
                            (ForwardVector * VaultStartOffset.Y) + //# Y Offset
                            (UpVector * VaultStartOffset.Z); //# Z Offset

                        FVector FinalLandPos = LandPos +
                            (RightVector * VaultEndOffset.X) + //# X Offset
                            (ForwardVector * VaultEndOffset.Y) + //# Y Offset
                            (UpVector * VaultEndOffset.Z); //# Z Offset

                        //# 디버그는 Offset 적용 안함
                        DrawDebugSphere(World, HandPos, 10.f, 12, FColor::Yellow, false, 1.f);
                        DrawDebugSphere(World, LandPos, 10.f, 12, FColor::Green, false, 1.f);

                        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultStartName, FinalHandPos, TargetRotator);
                        MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(VaultEndName, FinalLandPos, TargetRotator);
                    }
                }

                break;
            }
        }
    }

    return VaultWallInfo;
}

