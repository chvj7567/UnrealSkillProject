// Fill out your copyright notice in the Description page of Project Settings.

#include "SpyGrappleUIComponent.h"
#include "Data/SpyAssetNames.h"
#include "Manager/SpyAssetManager.h"
#include "Blueprint/UserWidget.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SpyGrappleUIComponent)

USpyGrappleUIComponent::USpyGrappleUIComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USpyGrappleUIComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
	if (Owner == nullptr)
		return;

	APawn* Pawn = Cast<APawn>(Owner);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PC == nullptr || PC->IsLocalController() == false)
		return;

	TSubclassOf<UUserWidget> WidgetClass =
        USpyAssetManager::GetSubclassByName<UUserWidget>(SpyAssetNames::GrapplePromptWidget);
    if (WidgetClass == nullptr) return;

    PromptWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
    if (PromptWidget)
    {
        PromptWidget->AddToViewport();
        PromptWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

void USpyGrappleUIComponent::InjectGrappleHost(TScriptInterface<ISpyGrappleHost> InHost)
{
	GrappleHost = InHost;

	ISpyGrappleHost* Host = GrappleHost.GetInterface();
	if (Host == nullptr)
		return;

	//# 구독이 BeginPlay 보다 늦어지므로 로컬 인지 타깃으로 1회 동기화한다.
	//# CurrentGrappleTarget 은 레플리케이트값이라 시뮬레이티드 프록시에서 쓰면 하이라이트가 영구 유출된다.
	Host->OnGrappleTargetChanged().AddUniqueDynamic(this, &USpyGrappleUIComponent::OnTargetChanged);
	OnTargetChanged(Host->GetLocalCachedTarget());
}

void USpyGrappleUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ISpyGrappleHost* Host = GrappleHost.GetInterface())
	{
		Host->OnGrappleTargetChanged().RemoveDynamic(this, &USpyGrappleUIComponent::OnTargetChanged);
	}

	if (PromptWidget)
	{
        PromptWidget->RemoveFromParent();
        PromptWidget = nullptr;
    }
    Super::EndPlay(EndPlayReason);
}

void USpyGrappleUIComponent::OnTargetChanged(AActor* NewTarget)
{
    if (NewTarget)
    {
        SetMeshCustomDepth(OldTarget.Get(), false);
        SetMeshCustomDepth(NewTarget, true);

        if (PromptWidget)
        {
            PromptWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        OldTarget = NewTarget;
    }
    else
    {
        SetMeshCustomDepth(OldTarget.Get(), false);

        if (PromptWidget)
        {
            PromptWidget->SetVisibility(ESlateVisibility::Hidden);
        }
        OldTarget = nullptr;
    }
}

void USpyGrappleUIComponent::SetMeshCustomDepth(AActor* Actor, bool bEnable) const
{
    if (Actor == nullptr) return;
    if (UMeshComponent* Mesh = Actor->FindComponentByClass<UMeshComponent>())
    {
        Mesh->SetRenderCustomDepth(bEnable);
        if (bEnable)
        {
            Mesh->SetCustomDepthStencilValue(1);
        }
    }
}
