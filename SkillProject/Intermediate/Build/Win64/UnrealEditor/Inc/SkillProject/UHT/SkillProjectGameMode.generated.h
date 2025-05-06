// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SkillProjectGameMode.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SKILLPROJECT_SkillProjectGameMode_generated_h
#error "SkillProjectGameMode.generated.h already included, missing '#pragma once' in SkillProjectGameMode.h"
#endif
#define SKILLPROJECT_SkillProjectGameMode_generated_h

#define FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_12_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesASkillProjectGameMode(); \
	friend struct Z_Construct_UClass_ASkillProjectGameMode_Statics; \
public: \
	DECLARE_CLASS(ASkillProjectGameMode, AGameModeBase, COMPILED_IN_FLAGS(0 | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/SkillProject"), SKILLPROJECT_API) \
	DECLARE_SERIALIZER(ASkillProjectGameMode)


#define FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_12_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ASkillProjectGameMode(ASkillProjectGameMode&&); \
	ASkillProjectGameMode(const ASkillProjectGameMode&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(SKILLPROJECT_API, ASkillProjectGameMode); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ASkillProjectGameMode); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ASkillProjectGameMode) \
	SKILLPROJECT_API virtual ~ASkillProjectGameMode();


#define FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_9_PROLOG
#define FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_12_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_12_INCLASS_NO_PURE_DECLS \
	FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h_12_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SKILLPROJECT_API UClass* StaticClass<class ASkillProjectGameMode>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SkillProject_Source_SkillProject_SkillProjectGameMode_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
