﻿#pragma once
#include "MassActorSubsystem.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityTraitBase.h"
#include "MassLODFragments.h"
#include "MassMovementFragments.h"
#include "MassSimulationLOD.h"
#include "MSUnitFragments.h"
#include "Example/MassSimpleMovementTrait.h"
#include "MSUnitTrait.generated.h"

UCLASS()
class UMSUnitTrait : public UMassEntityTraitBase
{
    GENERATED_BODY()

protected:

    virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override
    {
        BuildContext.AddFragment<FTransformFragment>();
        BuildContext.AddFragment<FMassHealthFragment>();

        // Fragments required by MassVisualizationTrait
        BuildContext.AddFragment<FMassActorFragment>();
        BuildContext.AddFragment<FMassViewerInfoFragment>();

        // Movement
        BuildContext.AddTag<FMassSimpleMovementTag>();

        // LOD
        BuildContext.AddTag<FMassCollectLODViewerInfoTag>();
    }
};