// Fill out your copyright notice in the Description page of Project Settings.


#include "StateTreeTask_Move.h"

#include "MassMovementFragments.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"

namespace MS::StateTreeTask_Move::Tweakables
{
	bool bSkipMoveTask = true;

	FAutoConsoleVariableRef CVars[] =
	{
		FAutoConsoleVariableRef(TEXT("ms.SkipMoveTask"), bSkipMoveTask, TEXT("Skip FStateTreeTask_Move"), ECVF_Cheat),
	};
}


bool FStateTreeTask_Move::Link(FStateTreeLinker& Linker)
{
    Linker.LinkExternalData(VelocityHandle);
    Linker.LinkExternalData(MassSignalSubsystemHandle);
    return true;
}

EStateTreeRunStatus FStateTreeTask_Move::EnterState(FStateTreeExecutionContext& Context,
    const FStateTreeTransitionResult& Transition) const
{
	if (MS::StateTreeTask_Move::Tweakables::bSkipMoveTask)
	{
		return EStateTreeRunStatus::Succeeded;
	}

    const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
    FMassVelocityFragment& MassVelocityFragment = MassContext.GetExternalData(VelocityHandle);
    MassVelocityFragment.Value = { FMath::FRandRange(-100.f, 100.f), FMath::FRandRange(-100.f, 100.f), 0 };

    UMassSignalSubsystem& MassSignalSubsystem = MassContext.GetExternalData(MassSignalSubsystemHandle);
    MassSignalSubsystem.DelaySignalEntity(UE::Mass::Signals::StandTaskFinished, MassContext.GetEntity(), 2);

    return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeTask_Move::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
    return EStateTreeRunStatus::Succeeded;
}
