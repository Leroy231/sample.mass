#include "MassDecrementHealthProcessor.h"

#include "MassSample/Unit/MSUnitFragments.h"

UMassDecrementHealthProcessor::UMassDecrementHealthProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
}

void UMassDecrementHealthProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMSHealthFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassDecrementHealthProcessor::Execute(FMassEntityManager& EntityManager,
													FMassExecutionContext& Context)
{
	// Decrement health only once a second.
	const double WorldTimeSeconds = Context.GetWorld()->GetTimeSeconds();
	if (WorldTimeSeconds - LastWorldTimeSecondsWhenDecremented < 1.f)
	{
		return;
	}

	LastWorldTimeSecondsWhenDecremented = WorldTimeSeconds;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Context)
	{
		const int32 NumEntities = Context.GetNumEntities();

		const TArrayView<FMSHealthFragment> HealthList = Context.GetMutableFragmentView<FMSHealthFragment>();

		for (int32 EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex)
		{
			FMSHealthFragment& HealthFragment = HealthList[EntityIndex];
			HealthFragment.Health -= 1;
		}
	});
}
