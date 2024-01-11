#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"

#include "MSUnitFragments.generated.h"

USTRUCT()
struct MASSSAMPLE_API FMassHealthFragment : public FMassFragment
{
    GENERATED_BODY()

    int32 Value = 100;
};
