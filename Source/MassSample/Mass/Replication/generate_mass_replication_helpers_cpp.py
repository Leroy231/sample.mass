# Note we indent this file with tabs instead of spaces so that the generated C++ will match UE C++ coding standards.

import json
from string import Template
from generate_mass_replication_helpers_common import *
import os

script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)

replication_config = json.load(open(os.path.join(script_dir, 'MassReplicationConfig.json')))

outl(f"""{generated_file_header}

#include "MassReplicationHelpersGenerated.h"

#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"

""")

for entity in replication_config['Entities']:
	fragments_short = [fragment[5:-8] for fragment in replication_config['Entities'][entity]['Fragments']]
	handlers = replication_config['Entities'][entity]['ClientBubbleAdditionalHandlers'] + fragments_short

	template = f"""
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void F{entity}ClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{{
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{{
{"\n".join([f"\t\t{handler}Handler.AddRequirementsForSpawnQuery(InQuery);" for handler in handlers])}
	}};

	auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
	{{
{"\n".join([f"\t\t{handler}Handler.CacheFragmentViewsForSpawnQuery(InExecContext);" for handler in handlers])}
	}};

	auto SetSpawnedEntityData = [this](const FMassEntityView&, const FReplicated{entity}Agent& ReplicatedEntity, const int32 EntityIdx)
	{{
{"\n".join([f"\t\t{handler}Handler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicated{replicated_data_getter(handler)}Data());" for handler in handlers])}
	}};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicated{entity}Agent& Item)
	{{
		PostReplicatedChangeEntity(EntityView, Item);
	}};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

{"\n".join([f"\t{handler}Handler.ClearFragmentViewsForSpawnQuery();" for handler in handlers])}
}}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE
	"""

	outl(template)

	set_handler_data = "\n".join([f"\t{handler}Handler.SetModifiedEntityData(EntityView, Item.GetReplicated{replicated_data_getter(handler)}Data());" for handler in handlers])

	template = Template("""
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void F${entity}ClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicated${entity}Agent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void F${entity}ClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicated${entity}Agent& Item) const
{
${set_handler_data}
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

A${entity}ClientBubbleInfo::A${entity}ClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Serializers.Add(&BubbleSerializer);
}

void A${entity}ClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
	DOREPLIFETIME_WITH_PARAMS_FAST(A${entity}ClientBubbleInfo, BubbleSerializer, SharedParams);
}
	""")

	outl(template.substitute(entity=entity, set_handler_data=set_handler_data))

	template = Template("""
void U${entity}Replicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
$add_requirements_body
}

void U${entity}Replicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE

$handler_defs
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;

	auto CacheViewsCallback = [&RepSharedFrag, $handler_refs](FMassExecutionContext& Context)
	{
$cache_fragment_views
		RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		check(RepSharedFrag);
	};

	auto AddEntityCallback = [&RepSharedFrag, $handler_refs](FMassExecutionContext& Context, const int32 EntityIdx, FReplicated${entity}Agent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
	{
		A${entity}ClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<A${entity}ClientBubbleInfo>(ClientHandle);

$add_entity

		return UnitBubbleInfo.GetBubbleSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
	};

	auto ModifyEntityCallback = [&RepSharedFrag, $handler_refs](FMassExecutionContext&, const int32 EntityIdx, const EMassLOD::Type, const double, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		A${entity}ClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<A${entity}ClientBubbleInfo>(ClientHandle);

		auto& Bubble = UnitBubbleInfo.GetBubbleSerializer().Bubble;

$modify_entity
	};

	auto RemoveEntityCallback = [&RepSharedFrag](FMassExecutionContext&, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		A${entity}ClientBubbleInfo& UnitBubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<A${entity}ClientBubbleInfo>(ClientHandle);

		UnitBubbleInfo.GetBubbleSerializer().Bubble.RemoveAgentChecked(Handle);
	};

	CalculateClientReplication<F${entity}FastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
	""")

	outl(template.substitute(
		add_requirements_body = "\n".join(["\tFMassReplicationProcessor%sHandler::AddRequirements(EntityQuery);" % (replicated_data_getter(handler)) for handler in handlers]),
		entity = entity,
		handler_defs = "\n".join(["\tFMassReplicationProcessor%sHandler %sHandler;" % (replicated_data_getter(handler), replicated_data_getter(handler)) for handler in handlers]), 
		handler_refs = ", ".join(["&%sHandler" % (replicated_data_getter(handler)) for handler in handlers]), 
		cache_fragment_views = "\n".join(["\t\t%sHandler.CacheFragmentViews(Context);" % (replicated_data_getter(handler)) for handler in handlers]), 
		add_entity = "\n".join(["\t\t%sHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicated%sDataMutable());" % (replicated_data_getter(handler), replicated_data_getter(handler)) for handler in handlers]), 
		modify_entity = "\n".join(["\t\t%sHandler.ModifyEntity<F%sFastArrayItem>(Handle, EntityIdx, Bubble.Get%sHandlerMutable());" % (replicated_data_getter(handler), entity, handler) for handler in handlers]), 
	))

for fragment in replication_config['Fragments']:
	fragment_short = fragment[5:-8]

	properties = list(replication_config['Fragments'][fragment].keys())
	set_values = "\n".join(["\tInOutReplicated%sData.Set%s(%sFragment.%s);" % (fragment_short, property, fragment_short, property) for property in properties])

	template = Template("""
void FMassReplicationProcessor${fragment_short}Handler::AddRequirements(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMass${fragment_short}Fragment>(EMassFragmentAccess::ReadOnly);
}

void FMassReplicationProcessor${fragment_short}Handler::CacheFragmentViews(FMassExecutionContext& ExecContext)
{
	${fragment_short}List = ExecContext.GetMutableFragmentView<FMass${fragment_short}Fragment>();
}

void FMassReplicationProcessor${fragment_short}Handler::AddEntity(const int32 EntityIdx, FReplicatedAgent${fragment_short}Data& InOutReplicated${fragment_short}Data) const
{
	const FMass${fragment_short}Fragment& ${fragment_short}Fragment = ${fragment_short}List[EntityIdx];

${set_values}
}
	""")

	outl(template.substitute(fragment_short=fragment_short, set_values=set_values))

template = Template("""
void UMassReplicationBubbleRegistrationSubsystem::PostInitialize()
{
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

$register_bubbles
}
""")
register_bubbles = "\n".join(["\tReplicationSubsystem->RegisterBubbleInfoClass(A%sClientBubbleInfo::StaticClass());" % (entity) for entity in replication_config['Entities']])
outl(template.substitute(register_bubbles=register_bubbles))

write_to_file("MassReplicationHelpersGenerated.cpp")
