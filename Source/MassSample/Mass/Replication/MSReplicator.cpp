#include "MSReplicator.h"

#include "MassExecutionContext.h"
#include "MSReplicatedUnit.h"
#include "MassReplicationTransformHandlers.h"

//----------------------------------------------------------------------//
//  UMSReplicator
//----------------------------------------------------------------------//
void UMSReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
	FMassReplicationProcessorPositionYawHandler::AddRequirements(EntityQuery);
	FMassReplicationProcessorHealthHandler::AddRequirements(EntityQuery);
}

void UMSReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

	FMassReplicationProcessorPositionYawHandler PositionYawHandler;
	FMassReplicationProcessorHealthHandler HealthHandler;
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;

	auto CacheViewsCallback = [&RepSharedFrag, &PositionYawHandler, &HealthHandler](FMassExecutionContext& Context)
	{
		PositionYawHandler.CacheFragmentViews(Context);
		HealthHandler.CacheFragmentViews(Context);
		RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		check(RepSharedFrag);
	};

	auto AddEntityCallback = [&RepSharedFrag, &PositionYawHandler, &HealthHandler](FMassExecutionContext& Context, const int32 EntityIdx, FMSReplicatedUnitAgent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
	{
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		PositionYawHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedPositionYawDataMutable());
		HealthHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedHealthDataMutable());

		return UnitBubbleInfo.GetUnitSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
	};

	auto ModifyEntityCallback = [&RepSharedFrag, &PositionYawHandler, &HealthHandler](FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		auto& Bubble = UnitBubbleInfo.GetUnitSerializer().Bubble;

		PositionYawHandler.ModifyEntity<FMSUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetTransformHandlerMutable());
		HealthHandler.ModifyEntity<FMSUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetHealthHandlerMutable());
	};

	auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		UnitBubbleInfo.GetUnitSerializer().Bubble.RemoveAgentChecked(Handle);
	};

	CalculateClientReplication<FMSUnitFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}

//----------------------------------------------------------------------//
//  FMassReplicationProcessorHealthHandler
//----------------------------------------------------------------------//
void FMassReplicationProcessorHealthHandler::AddRequirements(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMSHealthFragment>(EMassFragmentAccess::ReadOnly);
}

void FMassReplicationProcessorHealthHandler::CacheFragmentViews(FMassExecutionContext& ExecContext)
{
	HealthList = ExecContext.GetMutableFragmentView<FMSHealthFragment>();
}

void FMassReplicationProcessorHealthHandler::AddEntity(const int32 EntityIdx, FReplicatedAgentHealthData& InOutReplicatedHealthData) const
{
	const FMSHealthFragment& HealthFragment = HealthList[EntityIdx];

	InOutReplicatedHealthData.SetHealth(HealthFragment.Health);
}
