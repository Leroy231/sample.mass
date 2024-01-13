#include "MassDecrementHealthProcessor.h"

#include "MassExecutionContext.h"
#include "MassSample/Unit/MSUnitFragments.h"

UMassDecrementHealthProcessor::UMassDecrementHealthProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
}

void UMassDecrementHealthProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMassHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassLifetimeFragment>(EMassFragmentAccess::ReadWrite);
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

		const TArrayView<FMassHealthFragment> HealthList = Context.GetMutableFragmentView<FMassHealthFragment>();
		const TArrayView<FMassLifetimeFragment> LifetimeList = Context.GetMutableFragmentView<FMassLifetimeFragment>();

		for (int32 EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex)
		{
			FMassHealthFragment& HealthFragment = HealthList[EntityIndex];
			FMassLifetimeFragment& LifetimeFragment = LifetimeList[EntityIndex];

			HealthFragment.Value -= 1;
			HealthFragment.bIsBleeding = !HealthFragment.bIsBleeding;

			LifetimeFragment.Value += 1.f;
		}
	});
}
