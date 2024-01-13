#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"

#include "MSUnitFragments.generated.h"

USTRUCT()
struct MASSSAMPLE_API FMassHealthFragment : public FMassFragment
{
    GENERATED_BODY()

    int32 Value = 100;
    bool bIsBleeding = false;
};

USTRUCT()
struct MASSSAMPLE_API FMassLifetimeFragment : public FMassFragment
{
    GENERATED_BODY()

    float Value = 0.f;
};
