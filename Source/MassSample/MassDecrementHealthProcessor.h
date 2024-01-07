#pragma once

#include "MassProcessor.h"

#include "MassDecrementHealthProcessor.generated.h"

UCLASS()
class MASSSAMPLE_API UMassDecrementHealthProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassDecrementHealthProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	double LastWorldTimeSecondsWhenDecremented = -1;
};
