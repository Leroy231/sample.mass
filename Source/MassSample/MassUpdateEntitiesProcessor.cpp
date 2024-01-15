#include "MassUpdateEntitiesProcessor.h"

#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "Mass/Replication/MassReplicationHelpersGenerated.h"
#include "MassSample/Unit/MSUnitFragments.h"

UMassUpdateEntitiesProcessor::UMassUpdateEntitiesProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
}

void UMassUpdateEntitiesProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMassHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassLifetimeFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassReplicationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassReplicatedAgentFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassNetworkIDFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassUpdateEntitiesProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Run only once a second.
	//const double WorldTimeSeconds = Context.GetWorld()->GetTimeSeconds();
	//if (WorldTimeSeconds - LastWorldTimeSecondsWhenRan < 1.f)
	//{
	//	return;
	//}

	//LastWorldTimeSecondsWhenRan = WorldTimeSeconds;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		const int32 NumEntities = Context.GetNumEntities();

		const TArrayView<FMassHealthFragment> HealthList = Context.GetMutableFragmentView<FMassHealthFragment>();
		const TArrayView<FMassLifetimeFragment> LifetimeList = Context.GetMutableFragmentView<FMassLifetimeFragment>();
		const TArrayView<FMassReplicationLODFragment> ReplicationLODList = Context.GetMutableFragmentView<FMassReplicationLODFragment>();
		//const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();

		for (int32 EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIndex);

			FMassHealthFragment& HealthFragment = HealthList[EntityIndex];
			FMassLifetimeFragment& LifetimeFragment = LifetimeList[EntityIndex];
			const FMassReplicationLODFragment& ReplicationLODFragment = ReplicationLODList[EntityIndex];
			//FTransformFragment& TransformFragment = TransformList[EntityIndex];
			//FTransform& Transform = TransformFragment.GetMutableTransform();

			UE_VLOG(this, LogMass, Verbose, TEXT("[%s] Entity [%s] Has FMassInReplicationGridTag: %d, ReplicationLOD: %s"), *StaticClass()->GetName(), *Entity.DebugGetDescription(), Context.DoesArchetypeHaveTag<FMassInReplicationGridTag>(), *UEnum::GetDisplayValueAsText(ReplicationLODFragment.LOD).ToString());
			if (bIsEnabled)
			{
				HealthFragment.Value -= 1;
				//HealthFragment.bIsBleeding = !HealthFragment.bIsBleeding;
				//LifetimeFragment.Value += 0.1f;
				//if (HealthFragment.bIsBleeding)
				//{
				//	HealthFragment.Value -= 1;
				//	
				//}
			}

			//if (Entity.Index == 97)
			//{
			//	UE_LOG(LogTemp, Warning, TEXT("Entity [%s] new health: %d, new lifetime: %.1f"), *Entity.DebugGetDescription(), HealthFragment.Value, LifetimeFragment.Value);
			//}

		}
	});

	const TArray<FViewerInfo>& AllViewersInfo = UWorld::GetSubsystem<UMassLODSubsystem>(Context.GetWorld())->GetViewers();
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(Context.GetWorld());
	const TArray<FMassClientHandle>& ClientHandles = ReplicationSubsystem->GetClientReplicationHandles();
	for (const FMassClientHandle ClientHandle : ClientHandles)
	{
		if (ReplicationSubsystem->IsValidClientHandle(ClientHandle) == false)
		{
			continue;
		}

		FMassClientReplicationInfo& ClientReplicationInfo = ReplicationSubsystem->GetMutableClientReplicationInfoChecked(ClientHandle);

		// Figure out all viewer of this client
		TArray<FViewerInfo> Viewers;
		for (const FMassViewerHandle ClientViewerHandle : ClientReplicationInfo.Handles)
		{
			const FViewerInfo* ViewerInfo = AllViewersInfo.FindByPredicate([ClientViewerHandle](const FViewerInfo& ViewerInfo) { return ClientViewerHandle == ViewerInfo.Handle; });
			if (ensureMsgf(ViewerInfo, TEXT("Expecting to find the client viewer handle in the all viewers info list")))
			{
				Viewers.Add(*ViewerInfo);
			}
			UE_VLOG(this, LogMass, Verbose, TEXT("[%s] Client [i: %d sn: %d] Viewer [i: %d sn: %d], PlayerController: %s"), *StaticClass()->GetName(), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber(), ClientViewerHandle.GetIndex(), ClientViewerHandle.GetSerialNumber(), *ViewerInfo->PlayerController->GetName());
			UE_VLOG_LOCATION(this, LogMass, Verbose, ViewerInfo->Location, 10.f, FColor::Red, TEXT("Client [i: %d sn: %d] Viewer [i: %d sn: %d], PlayerController: %s"), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber(), ClientViewerHandle.GetIndex(), ClientViewerHandle.GetSerialNumber(), *ViewerInfo->PlayerController->GetName());
		}

		// Prepare LOD collector and calculator
		// Remember the max LOD distance from each
		float MaxLODDistance = 0.0f;
		EntityManager.ForEachSharedFragment<FMassReplicationSharedFragment>([&Viewers,&MaxLODDistance](FMassReplicationSharedFragment& RepSharedFragment)
		{
			RepSharedFragment.LODCollector.PrepareExecution(Viewers);
			RepSharedFragment.LODCalculator.PrepareExecution(Viewers);
			MaxLODDistance = FMath::Max(MaxLODDistance, RepSharedFragment.LODCalculator.GetMaxLODDistance());
		});

		EntityQuery.ForEachEntityChunk(EntityManager, Context, [&ClientReplicationInfo, this](FMassExecutionContext& Context)
		{
			const TConstArrayView<FMassReplicationLODFragment> ViewerLODList = Context.GetFragmentView<FMassReplicationLODFragment>();
			const TConstArrayView<FMassReplicatedAgentFragment> ReplicatedAgentList = Context.GetFragmentView<FMassReplicatedAgentFragment>();
			const TConstArrayView<FMassNetworkIDFragment> NetworkIDList = Context.GetFragmentView<FMassNetworkIDFragment>();
			const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();

			const int32 NumEntities = Context.GetNumEntities();
			for (int EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
				const FMassReplicatedAgentFragment& AgentFragment = ReplicatedAgentList[EntityIdx];
				const FMassReplicationLODFragment& LODFragment = ViewerLODList[EntityIdx];
				const FMassNetworkIDFragment& NetworkIDFragment = NetworkIDList[EntityIdx];
				FTransformFragment& TransformFragment = TransformList[EntityIdx];

				const bool bHasAgent = ClientReplicationInfo.AgentsData.Contains(Entity);
				UE_VLOG(this, LogMass, Verbose, TEXT("[%s] Entity [%s] bHasAgent: %d, ReplicationLOD: %s, AgentLOD: %s, NetID: %d"), *StaticClass()->GetName(), *Entity.DebugGetDescription(), bHasAgent, *UEnum::GetDisplayValueAsText(LODFragment.LOD).ToString(), *UEnum::GetDisplayValueAsText(AgentFragment.AgentData.LOD).ToString(), NetworkIDFragment.NetID.GetValue());
				UE_VLOG_LOCATION(this, LogMass, Verbose, TransformFragment.GetTransform().GetLocation(), 10.f, FColor::Red, TEXT("Entity [%s] bHasAgent: %d, ReplicationLOD: %s, AgentLOD: %s, NetID: %d"), *Entity.DebugGetDescription(), bHasAgent, *UEnum::GetDisplayValueAsText(LODFragment.LOD).ToString(), *UEnum::GetDisplayValueAsText(AgentFragment.AgentData.LOD).ToString(), NetworkIDFragment.NetID.GetValue());
			}
		});

		UE_VLOG(this, LogMass, Verbose, TEXT("[%s] Client [i: %d sn: %d] MaxLODDistance: %.1f, HandledEntitiesCount: %d, AgentsDataCount: %d"), *StaticClass()->GetName(), ClientHandle.GetIndex(), ClientHandle.GetSerialNumber(), MaxLODDistance, ClientReplicationInfo.HandledEntities.Num(), ClientReplicationInfo.AgentsData.Num());
	}
}

//----------------------------------------------------------------------//
// UMassNetworkIDEntityMapInitializer 
//----------------------------------------------------------------------//
UMassNetworkIDEntityMapInitializer::UMassNetworkIDEntityMapInitializer()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Standalone | EProcessorExecutionFlags::Server);
	ObservedType = FMassNetworkIDFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
}

void UMassNetworkIDEntityMapInitializer::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMassNetworkIDFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddSubsystemRequirement<UMassReplicationSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UMassNetworkIDEntityMapInitializer::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	QUICK_SCOPE_CYCLE_COUNTER(MassProcessor_InitNetworkID_Run);

	const UWorld* World = EntityManager.GetWorld();
	const ENetMode NetMode = World->GetNetMode();

	TMap<FMassNetworkID, FMassEntityHandle>& NetworkIDEntityMap = World->GetSubsystem<UMassReplicationNetworkIDEntityMapSubsystem>()->NetworkIDEntityMap;

	if (NetMode != NM_Client)
	{
#if UE_REPLICATION_COMPILE_SERVER_CODE
		EntityQuery.ForEachEntityChunk(EntityManager, Context, [&NetworkIDEntityMap](FMassExecutionContext& Context)
			{
				const TArrayView<FMassNetworkIDFragment> NetworkIDList = Context.GetMutableFragmentView<FMassNetworkIDFragment>();
				const int32 NumEntities = Context.GetNumEntities();

				for (int32 Idx = 0; Idx < NumEntities; ++Idx)
				{
					const FMassNetworkID NetID = NetworkIDList[Idx].NetID;
					check(NetID.IsValid());
					NetworkIDEntityMap.Add(NetID, Context.GetEntity(Idx));

				}
			});
#endif //UE_REPLICATION_COMPILE_SERVER_CODE
	}
}

/*static*/ AActor* UMSBPFunctionLibrary::GetActorForEntityWithNetID(const UObject* WorldContextObject, const int32 EntityNetID)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	const UMassReplicationNetworkIDEntityMapSubsystem* NetworkIDEntityMapSubsystem = UWorld::GetSubsystem<UMassReplicationNetworkIDEntityMapSubsystem>(World);
	const FMassEntityHandle* Entity = NetworkIDEntityMapSubsystem->NetworkIDEntityMap.Find(FMassNetworkID(static_cast<uint32>(EntityNetID)));

	if (!Entity || !Entity->IsValid())
	{
		return nullptr;
	}

	const FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(World);
	if (!EntityManager)
	{
		return nullptr;
	}

	if (FMassActorFragment* ActorFragment = EntityManager->GetFragmentDataPtr<FMassActorFragment>(*Entity))
	{
		return ActorFragment->GetMutable();
	}

	return nullptr;
}
