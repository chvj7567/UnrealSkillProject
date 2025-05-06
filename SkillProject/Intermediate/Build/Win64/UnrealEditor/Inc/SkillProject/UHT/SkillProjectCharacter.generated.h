// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "SkillProjectCharacter.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef SKILLPROJECT_SkillProjectCharacter_generated_h
#error "SkillProjectCharacter.generated.h already included, missing '#pragma once' in SkillProjectCharacter.h"
#endif
#define SKILLPROJECT_SkillProjectCharacter_generated_h

#define FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_21_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesASkillProjectCharacter(); \
	friend struct Z_Construct_UClass_ASkillProjectCharacter_Statics; \
public: \
	DECLARE_CLASS(ASkillProjectCharacter, ACharacter, COMPILED_IN_FLAGS(0 | CLASS_Config), CASTCLASS_None, TEXT("/Script/SkillProject"), NO_API) \
	DECLARE_SERIALIZER(ASkillProjectCharacter)


#define FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_21_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ASkillProjectCharacter(ASkillProjectCharacter&&); \
	ASkillProjectCharacter(const ASkillProjectCharacter&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, ASkillProjectCharacter); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(ASkillProjectCharacter); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(ASkillProjectCharacter) \
	NO_API virtual ~ASkillProjectCharacter();


#define FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_18_PROLOG
#define FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_21_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_21_INCLASS_NO_PURE_DECLS \
	FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h_21_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> SKILLPROJECT_API UClass* StaticClass<class ASkillProjectCharacter>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_SkillProject_Source_SkillProject_SkillProjectCharacter_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
