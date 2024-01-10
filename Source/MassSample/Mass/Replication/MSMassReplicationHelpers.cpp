#include "MSMassReplicationHelpers.h"

#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
    auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
    {
        TransformHandler.AddRequirementsForSpawnQuery(InQuery);
        HealthHandler.AddRequirementsForSpawnQuery(InQuery);
    };

    auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
    {
        TransformHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
        HealthHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
    };

    auto SetSpawnedEntityData = [this](const FMassEntityView& EntityView, const FMSReplicatedUnitAgent& ReplicatedEntity, const int32 EntityIdx)
    {
        TransformHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedPositionYawData());
        HealthHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedHealthData());
    };

    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FMSReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

    TransformHandler.ClearFragmentViewsForSpawnQuery();
    HealthHandler.ClearFragmentViewsForSpawnQuery();
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::FMSUnitClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
    auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FMSReplicatedUnitAgent& Item)
    {
        PostReplicatedChangeEntity(EntityView, Item);
    };

    PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FMSReplicatedUnitAgent& Item) const
{
    TransformHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedPositionYawData());
    HealthHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedHealthData());
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

AMSUnitClientBubbleInfo::AMSUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    Serializers.Add(&UnitSerializer);
}

void AMSUnitClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;

    // Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
    DOREPLIFETIME_WITH_PARAMS_FAST(AMSUnitClientBubbleInfo, UnitSerializer, SharedParams);
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
