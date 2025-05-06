// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SkillProject/SkillProjectGameMode.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSkillProjectGameMode() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
SKILLPROJECT_API UClass* Z_Construct_UClass_ASkillProjectGameMode();
SKILLPROJECT_API UClass* Z_Construct_UClass_ASkillProjectGameMode_NoRegister();
UPackage* Z_Construct_UPackage__Script_SkillProject();
// End Cross Module References

// Begin Class ASkillProjectGameMode
void ASkillProjectGameMode::StaticRegisterNativesASkillProjectGameMode()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ASkillProjectGameMode);
UClass* Z_Construct_UClass_ASkillProjectGameMode_NoRegister()
{
	return ASkillProjectGameMode::StaticClass();
}
struct Z_Construct_UClass_ASkillProjectGameMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "SkillProjectGameMode.h" },
		{ "ModuleRelativePath", "SkillProjectGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ASkillProjectGameMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_ASkillProjectGameMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AGameModeBase,
	(UObject* (*)())Z_Construct_UPackage__Script_SkillProject,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ASkillProjectGameMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ASkillProjectGameMode_Statics::ClassParams = {
	&ASkillProjectGameMode::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x008802ACu,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ASkillProjectGameMode_Statics::Class_MetaDataParams), Z_Construct_UClass_ASkillProjectGameMode_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_ASkillProjectGameMode()
{
	if (!Z_Registration_Info_UClass_ASkillProjectGameMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ASkillProjectGameMode.OuterSingleton, Z_Construct_UClass_ASkillProjectGameMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ASkillProjectGameMode.OuterSingleton;
}
template<> SKILLPROJECT_API UClass* StaticClass<ASkillProjectGameMode>()
{
	return ASkillProjectGameMode::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(ASkillProjectGameMode);
ASkillProjectGameMode::~ASkillProjectGameMode() {}
// End Class ASkillProjectGameMode

// Begin Registration
struct Z_CompiledInDeferFile_FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ASkillProjectGameMode, ASkillProjectGameMode::StaticClass, TEXT("ASkillProjectGameMode"), &Z_Registration_Info_UClass_ASkillProjectGameMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ASkillProjectGameMode), 952145715U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_3882021605(TEXT("/Script/SkillProject"),
	Z_CompiledInDeferFile_FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
