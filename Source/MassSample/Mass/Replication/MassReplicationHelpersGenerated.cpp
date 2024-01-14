// THIS IS GENERATED CODE. DO NOT MODIFY.
// REGENERATE WITH: python generate_mass_replication_helpers.py

UE_DISABLE_OPTIMIZATION

#include "MassReplicationHelpersGenerated.h"

#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"

DEFINE_LOG_CATEGORY(LogMassReplicationGenerated);


#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		TransformHandler.AddRequirementsForSpawnQuery(InQuery);
		HealthHandler.AddRequirementsForSpawnQuery(InQuery);
		LifetimeHandler.AddRequirementsForSpawnQuery(InQuery);
	};

	auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
	{
		TransformHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
		HealthHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
		LifetimeHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
	};

	auto SetSpawnedEntityData = [this](const FMassEntityView&, const FReplicatedMSUnitAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		TransformHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedPositionYawData());
		HealthHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedHealthData());
		LifetimeHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedLifetimeData());
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicatedMSUnitAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

	TransformHandler.ClearFragmentViewsForSpawnQuery();
	HealthHandler.ClearFragmentViewsForSpawnQuery();
	LifetimeHandler.ClearFragmentViewsForSpawnQuery();
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE
	

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicatedMSUnitAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMSUnitClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicatedMSUnitAgent& Item) const
{
	TransformHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedPositionYawData());
	HealthHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedHealthData());
	LifetimeHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedLifetimeData());
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

AMSUnitClientBubbleInfo::AMSUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Serializers.Add(&BubbleSerializer);
}

void AMSUnitClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
	DOREPLIFETIME_WITH_PARAMS_FAST(AMSUnitClientBubbleInfo, BubbleSerializer, SharedParams);
}
	

void UMSUnitReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
	FMassReplicationProcessorPositionYawHandler::AddRequirements(EntityQuery);
	FMassReplicationProcessorHealthHandler::AddRequirements(EntityQuery);
	FMassReplicationProcessorLifetimeHandler::AddRequirements(EntityQuery);
}

void UMSUnitReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

	FMassReplicationProcessorPositionYawHandler PositionYawHandler;
	FMassReplicationProcessorHealthHandler HealthHandler;
	FMassReplicationProcessorLifetimeHandler LifetimeHandler;
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;

	auto CacheViewsCallback = [&RepSharedFrag, &PositionYawHandler, &HealthHandler, &LifetimeHandler](FMassExecutionContext& Context)
	{
		PositionYawHandler.CacheFragmentViews(Context);
		HealthHandler.CacheFragmentViews(Context);
		LifetimeHandler.CacheFragmentViews(Context);
		RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		check(RepSharedFrag);
	};

	auto AddEntityCallback = [&RepSharedFrag, &PositionYawHandler, &HealthHandler, &LifetimeHandler](FMassExecutionContext& Context, const int32 EntityIdx, FReplicatedMSUnitAgent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
	{
		UE_VLOG(GWorld, LogMassReplicationGenerated, Verbose, TEXT("[UMSUnitReplicator::ProcessClientReplication][AddEntityCallback] Entity [%s] ClientHandle [i: %d sn: %d]"), *Context.GetEntity(EntityIdx).DebugGetDescription(), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber());
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		PositionYawHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedPositionYawDataMutable());
		HealthHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedHealthDataMutable());
		LifetimeHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedLifetimeDataMutable());

		return UnitBubbleInfo.GetBubbleSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
	};

	auto ModifyEntityCallback = [this, &RepSharedFrag, &PositionYawHandler, &HealthHandler, &LifetimeHandler](const FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type, const double, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		UE_VLOG(GWorld, LogMassReplicationGenerated, Verbose, TEXT("[UMSUnitReplicator::ProcessClientReplication][ModifyEntityCallback] Entity [%s], Agent [i: %d sn: %d], ClientHandle: [i: %d sn: %d]"), *Context.GetEntity(EntityIdx).DebugGetDescription(), Handle.GetIndex(), Handle.GetSerialNumber(), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber());
		check(Handle.IsValid());
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		auto& Bubble = UnitBubbleInfo.GetBubbleSerializer().Bubble;

		PositionYawHandler.ModifyEntity<FMSUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetTransformHandlerMutable());
		HealthHandler.ModifyEntity<FMSUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetHealthHandlerMutable());
		LifetimeHandler.ModifyEntity<FMSUnitFastArrayItem>(Handle, EntityIdx, Bubble.GetLifetimeHandlerMutable());
	};

	auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext&, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		check(Handle.IsValid());
		UE_VLOG(GWorld, LogMassReplicationGenerated, Verbose, TEXT("[UMSUnitReplicator::ProcessClientReplication][RemoveEntityCallback] Agent [i: %d sn: %d], ClientHandle: [i: %d sn: %d]"), Handle.GetIndex(), Handle.GetSerialNumber(), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber());
		AMSUnitClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMSUnitClientBubbleInfo>(ClientHandle);

		UnitBubbleInfo.GetBubbleSerializer().Bubble.RemoveAgentChecked(Handle);
	};

	CalculateClientReplication<FMSUnitFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
	

void FMassReplicationProcessorHealthHandler::AddRequirements(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMassHealthFragment>(EMassFragmentAccess::ReadOnly);
}

void FMassReplicationProcessorHealthHandler::CacheFragmentViews(FMassExecutionContext& ExecContext)
{
	HealthList = ExecContext.GetMutableFragmentView<FMassHealthFragment>();
}

void FMassReplicationProcessorHealthHandler::AddEntity(const int32 EntityIdx, FReplicatedAgentHealthData& InOutReplicatedHealthData) const
{
	const FMassHealthFragment& HealthFragment = HealthList[EntityIdx];

	InOutReplicatedHealthData.SetValue(HealthFragment.Value);
	InOutReplicatedHealthData.SetbIsBleeding(HealthFragment.bIsBleeding);
}
	

void FMassReplicationProcessorLifetimeHandler::AddRequirements(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMassLifetimeFragment>(EMassFragmentAccess::ReadOnly);
}

void FMassReplicationProcessorLifetimeHandler::CacheFragmentViews(FMassExecutionContext& ExecContext)
{
	LifetimeList = ExecContext.GetMutableFragmentView<FMassLifetimeFragment>();
}

void FMassReplicationProcessorLifetimeHandler::AddEntity(const int32 EntityIdx, FReplicatedAgentLifetimeData& InOutReplicatedLifetimeData) const
{
	const FMassLifetimeFragment& LifetimeFragment = LifetimeList[EntityIdx];

	InOutReplicatedLifetimeData.SetValue(LifetimeFragment.Value);
}
	

void UMassReplicationBubbleRegistrationSubsystem::PostInitialize()
{
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

	ReplicationSubsystem->RegisterBubbleInfoClass(AMSUnitClientBubbleInfo::StaticClass());
}

UE_ENABLE_OPTIMIZATION
