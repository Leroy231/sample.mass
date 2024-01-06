// Fill out your copyright notice in the Description page of Project Settings.

#include "MassSample.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "GameplayDebuggerCategory_ProjectM.h"
#endif // WITH_GAMEPLAY_DEBUGGER

class FMassSampleGameModuleImpl
	: public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("ProjectM", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_ProjectM::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 1);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FMassSampleGameModuleImpl, MassSample, "MassSample" );
