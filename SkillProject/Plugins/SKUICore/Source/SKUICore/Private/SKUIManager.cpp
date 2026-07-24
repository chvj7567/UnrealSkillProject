#include "SKUIManager.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Blueprint/UserWidget.h"
#include "SKUserWidget.h"
#include "SKAssetManager.h"
#include "SKAssetData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SKUIManager)

bool USKUIManager::ShouldCreateSubsystem(UObject* Outer) const
{
	//# 파생 클래스가 있으면 base 는 생성 안 함 (leaf 만 생성 → 인스턴스 분리 방지)
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(GetClass(), DerivedClasses, false);
	return DerivedClasses.Num() == 0;
}

void USKUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USKUIManager::Deinitialize()
{
	//# 뷰포트에 얹어 둔 persistent UI 를 모두 걷어낸다
	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		for (const TObjectPtr<USKUserWidget>& UserWidget : PersistentUIList)
		{
			if (IsValid(UserWidget))
			{
				GEngine->GameViewport->RemoveViewportWidgetContent(UserWidget->TakeWidget());
			}
		}
	}

	PersistentUIList.Empty();

	Super::Deinitialize();
}

USKUIManager* USKUIManager::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
		return nullptr;

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			//# 파생 포함 인스턴스 조회 — leaf(프로젝트 UIManager 서브클래스) 하나가 잡힌다
			//# (UE5.4+ 에서 GetSubsystemArray → GetSubsystemArrayCopy 로 개명, 값 반환)
			TArray<USKUIManager*> Subsystems = GI->GetSubsystemArrayCopy<USKUIManager>();
			checkf(Subsystems.Num() <= 1, TEXT("USKUIManager 인스턴스가 2개 이상 — ShouldCreateSubsystem 확인"));
			return Subsystems.Num() > 0 ? Subsystems[0] : nullptr;
		}
	}

	return nullptr;
}

void USKUIManager::OpenUI(FName InUIName)
{
	//# 패키지 빌드에서 BP 오브젝트(BP_X.BP_X)는 cook 시 stripped되므로 generated class(BP_X.BP_X_C) 경로로 로드
	const USKAssetData& AssetData = USKAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);
	FString ClassPathString = AssetPath.GetAssetPathString();
	if (ClassPathString.EndsWith(TEXT("_C")) == false)
	{
		ClassPathString.Append(TEXT("_C"));
	}
	FSoftObjectPath ClassPath(ClassPathString);

	FSKAssetAndDelegate LoadDelegate;
	LoadDelegate.BindLambda([this, InUIName](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr)
				return;

			if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				//# 이미 열려있는 UI 확인
				const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
					[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
					{
						return UserWidget->GetUIName() == InUIName;
					});

				if (FindOpenningUI)
				{
					//# 동일한 UI는 중복해서 띄우지 않음
					UE_LOG(LogTemp, Warning, TEXT("Already Opening UI: %s"), *InUIName.ToString());
					return;
				}

				//# 캐싱 중인 UI 확인
				const TObjectPtr<USKUserWidget>* FindCashingUI = CashingUIList.FindByPredicate(
					[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
					{
						return UserWidget->GetUIName() == InUIName;
					});

				if (FindCashingUI)
				{
					//# 캐싱 중인 UI이면 Open
					OpenUIList.Add(FindCashingUI->Get());
					FindCashingUI->Get()->AddToViewport();

					UE_LOG(LogTemp, Warning, TEXT("Cashing Opening UI: %s"), *InUIName.ToString());
					return;
				}

				//# UI 생성
				if (USKUserWidget* UserWidget = CreateWidget<USKUserWidget>(GetWorld(), UI))
				{
					UserWidget->SetUIName(InUIName);
					OpenUIList.Add(UserWidget);

					UserWidget->AddToViewport();

					UE_LOG(LogTemp, Warning, TEXT("New Opening UI: %s"), *InUIName.ToString());
				}
			}
		});

	USKAssetManager::LoadAssetAsync(ClassPath, LoadDelegate);
}

void USKUIManager::CloseUI(FName InUIName)
{
	UE_LOG(LogTemp, Warning, TEXT("CloseUI: %s"), *InUIName.ToString());

	if (OpenUIList.IsEmpty())
		return;

	//# 이미 열려있는 UI 확인
	const TObjectPtr<USKUserWidget>* FindOpenningUI = OpenUIList.FindByPredicate(
		[InUIName](TObjectPtr<USKUserWidget>& UserWidget)
		{
			return UserWidget->GetUIName() == InUIName;
		});

	if (FindOpenningUI)
	{
		OpenUIList.Remove(FindOpenningUI->Get());
		AddCashingUI(FindOpenningUI->Get());
		FindOpenningUI->Get()->RemoveFromParent();
	}
}

void USKUIManager::CloseLastUI()
{
	if (OpenUIList.IsEmpty())
		return;

	if (USKUserWidget* UserWidget = OpenUIList.Last())
	{
		OpenUIList.Pop();
		CloseUI(UserWidget->GetUIName());
	}
}

void USKUIManager::OpenSubUI(FName InUIName, UWidgetComponent* WidgetComponent, EWidgetSpace Space)
{
	const USKAssetData& AssetData = USKAssetManager::Get().GetAssetData();
	const FSoftObjectPath& AssetPath = AssetData.GetAssetPathByName(InUIName);
	FString ClassPathString = AssetPath.GetAssetPathString();
	if (ClassPathString.EndsWith(TEXT("_C")) == false)
	{
		ClassPathString.Append(TEXT("_C"));
	}
	FSoftObjectPath ClassPath(ClassPathString);

	if (WidgetComponent == nullptr)
		return;

	FSKAssetAndDelegate LoadDelegate;
	TWeakObjectPtr<UWidgetComponent> WeakWidget = WidgetComponent;
	LoadDelegate.BindLambda([InUIName, WeakWidget, Space](UObject* LoadedAsset)
		{
			if (LoadedAsset == nullptr || WeakWidget.IsValid() == false)
				return;

			if (TSubclassOf<USKUserWidget> UI = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName))
			{
				WeakWidget->SetWidgetClass(UI);
				WeakWidget->SetWidgetSpace(Space);
				WeakWidget->InitWidget();
				WeakWidget->SetVisibility(true);
			}
		});

	USKAssetManager::LoadAssetAsync(ClassPath, LoadDelegate);
}

void USKUIManager::AddCashingUI(USKUserWidget* UserWidget)
{
	//# 캐싱 수가 Max라면 오래된 UI 제거 후 추가
	if (MaxCashingUICount > 0)
	{
		for (USKUserWidget* CashingUserWIdget : CashingUIList)
		{
			if (CashingUserWIdget->GetUIName() == UserWidget->GetUIName())
				return;
		}

		CashingUIList.Add(UserWidget);

		if (CashingUIList.Num() >= MaxCashingUICount)
		{
			CashingUIList.RemoveAt(0);
		}
	}
}

USKUserWidget* USKUIManager::OpenPersistentUI(FName InUIName, int32 ZOrder)
{
	//# 이미 열려 있으면 그것을 돌려준다 (중복 생성 금지)
	for (const TObjectPtr<USKUserWidget>& Existing : PersistentUIList)
	{
		if (IsValid(Existing) && Existing->GetUIName() == InUIName)
		{
			UE_LOG(LogTemp, Warning, TEXT("Already Opening Persistent UI: %s"), *InUIName.ToString());
			return Existing;
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	//# 데디케이티드 서버 등 뷰포트가 없는 환경에서는 아무것도 하지 않는다
	if (GEngine == nullptr || GEngine->GameViewport == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] 뷰포트가 없어 Persistent UI 를 열지 않습니다: %s"), *InUIName.ToString());
		return nullptr;
	}

	//# 위젯 클래스 동기 로드 — 비동기로 하면 표시가 지연된다
	TSubclassOf<USKUserWidget> WidgetClass = USKAssetManager::GetSubclassByName<USKUserWidget>(InUIName);
	if (WidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("# [SKUIManager] Persistent UI 클래스를 찾을 수 없습니다: %s"), *InUIName.ToString());
		return nullptr;
	}

	//# GameInstance 를 아우터로 생성 — 월드가 파괴돼도 위젯이 살아남는다
	USKUserWidget* UserWidget = CreateWidget<USKUserWidget>(GameInstance, WidgetClass);
	if (UserWidget == nullptr)
	{
		return nullptr;
	}

	UserWidget->SetUIName(InUIName);

	//# AddToViewport 가 아니라 Slate 뷰포트 콘텐츠로 직접 얹는다 (월드 비의존)
	GEngine->GameViewport->AddViewportWidgetContent(UserWidget->TakeWidget(), ZOrder);

	PersistentUIList.Add(UserWidget);

	UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] New Persistent UI: %s (ZOrder %d)"), *InUIName.ToString(), ZOrder);

	return UserWidget;
}

void USKUIManager::ClosePersistentUI(FName InUIName)
{
	for (int32 Index = PersistentUIList.Num() - 1; Index >= 0; --Index)
	{
		TObjectPtr<USKUserWidget> UserWidget = PersistentUIList[Index];

		if (IsValid(UserWidget) == false)
		{
			PersistentUIList.RemoveAt(Index);
			continue;
		}

		if (UserWidget->GetUIName() != InUIName)
		{
			continue;
		}

		if (GEngine != nullptr && GEngine->GameViewport != nullptr)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(UserWidget->TakeWidget());
		}

		PersistentUIList.RemoveAt(Index);

		UE_LOG(LogTemp, Log, TEXT("# [SKUIManager] Close Persistent UI: %s"), *InUIName.ToString());
	}
}

bool USKUIManager::IsPersistentUIOpen(FName InUIName) const
{
	for (const TObjectPtr<USKUserWidget>& UserWidget : PersistentUIList)
	{
		if (IsValid(UserWidget) && UserWidget->GetUIName() == InUIName)
		{
			return true;
		}
	}

	return false;
}
