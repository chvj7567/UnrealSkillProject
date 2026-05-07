#include "SSpyCreateGADialog.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "SGameplayTagContainerCombo.h"
#include "PropertyCustomizationHelpers.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/SavePackage.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "UObject/UnrealType.h"
#include "AssetRegistry/AssetRegistryModule.h"

static EGameplayAbilityNetExecutionPolicy::Type ParseNetExecPolicy(const FString& S)
{
    if (S == TEXT("LocalOnly"))       return EGameplayAbilityNetExecutionPolicy::LocalOnly;
    if (S == TEXT("ServerInitiated")) return EGameplayAbilityNetExecutionPolicy::ServerInitiated;
    if (S == TEXT("ServerOnly"))      return EGameplayAbilityNetExecutionPolicy::ServerOnly;
    return EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

static EGameplayAbilityInstancingPolicy::Type ParseInstancingPolicy(const FString& S)
{
    if (S == TEXT("NonInstanced"))          return EGameplayAbilityInstancingPolicy::NonInstanced;
    if (S == TEXT("InstancedPerExecution")) return EGameplayAbilityInstancingPolicy::InstancedPerExecution;
    return EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

static EGameplayAbilityNetSecurityPolicy::Type ParseNetSecPolicy(const FString& S)
{
    if (S == TEXT("ServerOnlyExecution"))    return EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution;
    if (S == TEXT("ServerOnlyTermination"))  return EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
    if (S == TEXT("ServerOnly"))             return EGameplayAbilityNetSecurityPolicy::ServerOnly;
    return EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}

#define LOCTEXT_NAMESPACE "SSpyCreateGADialog"

TSharedRef<SWidget> SSpyCreateGADialog::MakeSectionHeader(const FText& Label)
{
    return SNew(SBorder)
        .BorderBackgroundColor(FLinearColor(0.08f, 0.08f, 0.08f, 1.f))
        .Padding(FMargin(6.f, 3.f))
        [
            SNew(STextBlock)
            .Text(Label)
            .Font(FAppStyle::GetFontStyle("BoldFont"))
        ];
}

TSharedRef<SWidget> SSpyCreateGADialog::MakeLabeledRow(
    const FText& Label, TSharedRef<SWidget> Content)
{
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
          .AutoWidth()
          .VAlign(VAlign_Center)
          .Padding(FMargin(4.f, 2.f))
          [
              SNew(SBox).WidthOverride(200.f)
              [
                  SNew(STextBlock).Text(Label)
              ]
          ]
        + SHorizontalBox::Slot()
          .FillWidth(1.f)
          .VAlign(VAlign_Center)
          .Padding(FMargin(0.f, 2.f, 4.f, 2.f))
          [
              Content
          ];
}

void SSpyCreateGADialog::BuildClassList()
{
    ClassList.Empty();
    ClassList.Add(UGameplayAbility::StaticClass());

    TArray<UClass*> Derived;
    GetDerivedClasses(UGameplayAbility::StaticClass(), Derived, true);
    Derived.RemoveAll([](UClass* C)
    {
        return C->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated)
            || C->GetName().StartsWith(TEXT("SKEL_"))
            || C->GetName().StartsWith(TEXT("REINST_"));
    });
    Derived.Sort([](const UClass& A, const UClass& B)
    {
        return A.GetName() < B.GetName();
    });
    ClassList.Append(Derived);
    SelectedClass = UGameplayAbility::StaticClass();
}

void SSpyCreateGADialog::Construct(const FArguments& InArgs)
{
    BuildClassList();

    NetExecOptions = {
        MakeShared<FString>(TEXT("LocalPredicted")),
        MakeShared<FString>(TEXT("LocalOnly")),
        MakeShared<FString>(TEXT("ServerInitiated")),
        MakeShared<FString>(TEXT("ServerOnly")),
    };
    InstancingOptions = {
        MakeShared<FString>(TEXT("NonInstanced")),
        MakeShared<FString>(TEXT("InstancedPerActor")),
        MakeShared<FString>(TEXT("InstancedPerExecution")),
    };
    NetSecOptions = {
        MakeShared<FString>(TEXT("ClientOrServer")),
        MakeShared<FString>(TEXT("ServerOnlyExecution")),
        MakeShared<FString>(TEXT("ServerOnlyTermination")),
        MakeShared<FString>(TEXT("ServerOnly")),
    };
    SelectedNetExecPolicy    = NetExecOptions[0];
    SelectedInstancingPolicy = InstancingOptions[1];
    SelectedNetSecPolicy     = NetSecOptions[0];

    ChildSlot
    [
        SNew(SScrollBox)

        + SScrollBox::Slot().Padding(FMargin(0.f, 4.f))
        [ MakeSectionHeader(LOCTEXT("SectionHeader", "기본 설정")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ParentClass", "Parent Class"),
                SNew(SComboBox<UClass*>)
                    .OptionsSource(&ClassList)
                    .InitiallySelectedItem(SelectedClass)
                    .OnSelectionChanged_Lambda([this](UClass* Item, ESelectInfo::Type)
                    {
                        SelectedClass = Item;
                    })
                    .OnGenerateWidget_Lambda([](UClass* C) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock)
                            .Text(FText::FromString(C ? C->GetName() : TEXT("None")));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedClass ? SelectedClass->GetName() : TEXT("선택..."));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("GAName", "GA Name"),
                SNew(SEditableTextBox)
                    .HintText(LOCTEXT("GANameHint", "예: MySkill  →  GA_MySkill"))
                    .OnTextChanged_Lambda([this](const FText& Text)
                    {
                        GAName = Text.ToString();
                        if (OutputPathText.IsValid())
                        {
                            OutputPathText->SetText(FText::FromString(FString::Printf(
                                TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_%s"), *GAName)));
                        }
                    })
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("OutputPath", "Output Path"),
                SAssignNew(OutputPathText, STextBlock)
                    .Text(FText::FromString(TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_")))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            )
        ]

        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionTags", "Tags")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("AbilityTags", "Ability Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return AbilityTagsContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { AbilityTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationOwnedTags", "Activation Owned Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationOwnedTagsContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationOwnedTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationRequiredTags", "Activation Required Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationRequiredTagsContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationRequiredTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("ActivationBlockedTags", "Activation Blocked Tags"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return ActivationBlockedTagsContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { ActivationBlockedTagsContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CancelAbilitiesWithTag", "Cancel Abilities With Tag"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return CancelAbilitiesWithTagContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { CancelAbilitiesWithTagContainer = C; }))
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("BlockAbilitiesWithTag", "Block Abilities With Tag"),
                SNew(SGameplayTagContainerCombo)
                    .TagContainer_Lambda([this]() { return BlockAbilitiesWithTagContainer; })
                    .OnTagContainerChanged(SGameplayTagContainerCombo::FOnTagContainerChanged::CreateLambda(
                        [this](const FGameplayTagContainer& C) { BlockAbilitiesWithTagContainer = C; }))
            )
        ]

        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionPolicies", "Policies")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("NetExec", "Net Execution Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&NetExecOptions)
                    .InitiallySelectedItem(SelectedNetExecPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedNetExecPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedNetExecPolicy.IsValid() ? *SelectedNetExecPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("Instancing", "Instancing Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&InstancingOptions)
                    .InitiallySelectedItem(SelectedInstancingPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedInstancingPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedInstancingPolicy.IsValid() ? *SelectedInstancingPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("NetSec", "Net Security Policy"),
                SNew(SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&NetSecOptions)
                    .InitiallySelectedItem(SelectedNetSecPolicy)
                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
                    {
                        if (Item.IsValid()) SelectedNetSecPolicy = Item;
                    })
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
                    {
                        return SNew(STextBlock).Text(FText::FromString(*Item));
                    })
                    [
                        SNew(STextBlock).Text_Lambda([this]() -> FText
                        {
                            return FText::FromString(
                                SelectedNetSecPolicy.IsValid() ? *SelectedNetSecPolicy : TEXT(""));
                        })
                    ]
            )
        ]

        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionCosts", "Costs")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CostGE", "Cost GE Class"),
                SNew(SClassPropertyEntryBox)
                    .MetaClass(UGameplayEffect::StaticClass())
                    .AllowNone(true)
                    .AllowAbstract(false)
                    .SelectedClass_Lambda([this]() -> const UClass* { return CostGEClass; })
                    .OnSetClass(FOnSetClass::CreateLambda(
                        [this](const UClass* C) { CostGEClass = const_cast<UClass*>(C); }))
            )
        ]

        + SScrollBox::Slot().Padding(FMargin(0.f, 8.f, 0.f, 0.f))
        [ MakeSectionHeader(LOCTEXT("SectionCooldowns", "Cooldowns")) ]

        + SScrollBox::Slot()
        [
            MakeLabeledRow(
                LOCTEXT("CooldownGE", "Cooldown GE Class"),
                SNew(SClassPropertyEntryBox)
                    .MetaClass(UGameplayEffect::StaticClass())
                    .AllowNone(true)
                    .AllowAbstract(false)
                    .SelectedClass_Lambda([this]() -> const UClass* { return CooldownGEClass; })
                    .OnSetClass(FOnSetClass::CreateLambda(
                        [this](const UClass* C) { CooldownGEClass = const_cast<UClass*>(C); }))
            )
        ]

        + SScrollBox::Slot().Padding(FMargin(4.f, 12.f))
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(LOCTEXT("CreateBtn", "Create GA"))
            .OnClicked(this, &SSpyCreateGADialog::OnCreateClicked)
        ]
    ];
}

bool SSpyCreateGADialog::ValidateInputs(FText& OutError) const
{
    if (SelectedClass == nullptr)
    {
        OutError = LOCTEXT("ErrNoClass", "Parent Class를 선택하세요.");
        return false;
    }
    if (GAName.IsEmpty())
    {
        OutError = LOCTEXT("ErrNoName", "GA 이름을 입력하세요.");
        return false;
    }
    for (TCHAR C : GAName)
    {
        if (FChar::IsAlnum(C) == false && C != TEXT('_'))
        {
            OutError = LOCTEXT("ErrBadName", "이름은 영숫자와 언더스코어(_)만 허용됩니다.");
            return false;
        }
    }
    FString PackagePath = FString::Printf(
        TEXT("/Game/Spy/Blueprints/GameplayAbilities/GA_%s"), *GAName);
    if (FPackageName::DoesPackageExist(PackagePath))
    {
        OutError = FText::Format(
            LOCTEXT("ErrExists", "'{0}' 에셋이 이미 존재합니다."),
            FText::FromString(PackagePath));
        return false;
    }
    return true;
}

FReply SSpyCreateGADialog::OnCreateClicked()
{
    FText Error;
    if (ValidateInputs(Error) == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, Error);
        return FReply::Handled();
    }

    //# 1. 패키지 경로 결정 및 생성
    const FString BlueprintName = FString::Printf(TEXT("GA_%s"), *GAName);
    const FString PackagePath   = FString::Printf(
        TEXT("/Game/Spy/Blueprints/GameplayAbilities/%s"), *BlueprintName);

    UPackage* Package = CreatePackage(*PackagePath);
    if (Package == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrPackage", "패키지 생성에 실패했습니다."));
        return FReply::Handled();
    }

    //# 2. Blueprint 생성
    UBlueprint* NewBP = FKismetEditorUtilities::CreateBlueprint(
        SelectedClass,
        Package,
        FName(*BlueprintName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass()
    );
    if (NewBP == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrBPCreate", "Blueprint 생성에 실패했습니다."));
        return FReply::Handled();
    }

    //# 3. 컴파일 — GeneratedClass 생성
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    if (NewBP->GeneratedClass == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrGeneratedClass", "Blueprint 컴파일 후 GeneratedClass가 없습니다."));
        return FReply::Handled();
    }

    //# 4. CDO에 설정값 반영 (FProperty 리플렉션으로 protected 멤버 접근)
    UGameplayAbility* CDO = NewBP->GeneratedClass->GetDefaultObject<UGameplayAbility>();
    if (CDO == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrCDOCast", "CDO를 UGameplayAbility로 캐스팅할 수 없습니다."));
        return FReply::Handled();
    }

    CDO->Modify();

    UClass* GAClass = CDO->GetClass();

    //# Tag containers (FStructProperty)
    auto SetTagContainer = [CDO, GAClass](const FName PropName, const FGameplayTagContainer& Value)
    {
        if (FStructProperty* Prop = FindFProperty<FStructProperty>(GAClass, PropName))
        {
            *Prop->ContainerPtrToValuePtr<FGameplayTagContainer>(CDO) = Value;
        }
    };

    SetTagContainer(FName(TEXT("AbilityTags")),            AbilityTagsContainer);
    SetTagContainer(FName(TEXT("ActivationOwnedTags")),    ActivationOwnedTagsContainer);
    SetTagContainer(FName(TEXT("ActivationRequiredTags")), ActivationRequiredTagsContainer);
    SetTagContainer(FName(TEXT("ActivationBlockedTags")),  ActivationBlockedTagsContainer);
    SetTagContainer(FName(TEXT("CancelAbilitiesWithTag")), CancelAbilitiesWithTagContainer);
    SetTagContainer(FName(TEXT("BlockAbilitiesWithTag")),  BlockAbilitiesWithTagContainer);

    //# Enum policies (TEnumAsByte → FByteProperty)
    auto SetByteProperty = [CDO, GAClass](const FName PropName, uint8 Value)
    {
        if (FByteProperty* Prop = FindFProperty<FByteProperty>(GAClass, PropName))
        {
            *Prop->ContainerPtrToValuePtr<uint8>(CDO) = Value;
        }
    };

    SetByteProperty(FName(TEXT("NetExecutionPolicy")),
        (uint8)ParseNetExecPolicy(
            SelectedNetExecPolicy.IsValid() ? *SelectedNetExecPolicy : TEXT("LocalPredicted")));
    SetByteProperty(FName(TEXT("InstancingPolicy")),
        (uint8)ParseInstancingPolicy(
            SelectedInstancingPolicy.IsValid() ? *SelectedInstancingPolicy : TEXT("InstancedPerActor")));
    SetByteProperty(FName(TEXT("NetSecurityPolicy")),
        (uint8)ParseNetSecPolicy(
            SelectedNetSecPolicy.IsValid() ? *SelectedNetSecPolicy : TEXT("ClientOrServer")));

    //# GE classes (TSubclassOf → FClassProperty)
    auto SetClassProperty = [CDO, GAClass](const FName PropName, UClass* Value)
    {
        if (FClassProperty* Prop = FindFProperty<FClassProperty>(GAClass, PropName))
        {
            Prop->SetObjectPropertyValue(Prop->ContainerPtrToValuePtr<void>(CDO), Value);
        }
    };

    SetClassProperty(FName(TEXT("CostGameplayEffectClass")),     CostGEClass);
    SetClassProperty(FName(TEXT("CooldownGameplayEffectClass")), CooldownGEClass);

    CDO->MarkPackageDirty();

    //# 5. 에셋 레지스트리 등록 (Content Browser에서 즉시 표시)
    FAssetRegistryModule::AssetCreated(NewBP);

    //# 6. 저장
    FString PackageFilename;
    if (FPackageName::TryConvertLongPackageNameToFilename(
            Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()) == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrFilename", "패키지 경로 변환에 실패했습니다."));
        return FReply::Handled();
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    const bool bSaved = UPackage::SavePackage(Package, NewBP, *PackageFilename, SaveArgs);
    if (bSaved == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("ErrSave", "Blueprint 저장에 실패했습니다. 디스크 공간 또는 파일 권한을 확인하세요."));
        return FReply::Handled();
    }

    //# 7. Blueprint 에디터 열기
    if (UAssetEditorSubsystem* EdSub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        EdSub->OpenEditorForAsset(NewBP);
    }

    FMessageDialog::Open(EAppMsgType::Ok,
        FText::Format(LOCTEXT("SuccessMsg", "'{0}' 이(가) 생성되었습니다."),
            FText::FromString(BlueprintName)));

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
