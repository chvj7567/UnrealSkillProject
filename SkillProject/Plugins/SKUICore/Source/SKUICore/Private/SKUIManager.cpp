#include "SKUIManager.h"
#include "SKUserWidget.h"
#include "SKAssetManager.h"
#include "SKAssetData.h"
#include "Blueprint/UserWidget.h"

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
