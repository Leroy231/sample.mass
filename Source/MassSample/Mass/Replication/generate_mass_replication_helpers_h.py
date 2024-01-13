import json
from generate_mass_replication_helpers_common import *
from string import Template
import os

script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)

outl("""#pragma once

#include "MassReplicationTypes.h"
#include "MassClientBubbleHandler.h"
#include "MassClientBubbleInfoBase.h"
#include "MassReplicationTransformHandlers.h"
#include "MassEntityView.h"
#include "MassReplicationProcessor.h"
""")

replication_config = json.load(open(os.path.join(script_dir, 'MassReplicationConfig.json')))
type_defaults = {"int32": "0", "bool": "false"}
module_macro = replication_config['ModuleMacro']

for include in replication_config['AdditionalIncludes']:
	outl('#include "%s"' % (include))
	
outl('\n#include "MassReplicationHelpersGenerated.generated.h"\n')

outl("""
template<typename AgentArrayItem>
class TClientBubbleHandlerBase2 : public TClientBubbleHandlerBase<AgentArrayItem>
{
public:
""", trimblanklines=True)

for fragment in replication_config['Fragments']:
	fragment_short = fragment[5:-8]
	outl("""
	template<typename T>
	friend class TMassClientBubble%sHandler;
	""" % (fragment_short), trimblanklines=True)

outl("};\n")

for fragment in replication_config['Fragments']:
	fragment_short = fragment[5:-8]
	outl("""
USTRUCT()
struct %s FReplicatedAgent%sData
{
	GENERATED_BODY()

	FReplicatedAgent%sData() = default;
	""" % (module_macro, fragment_short, fragment_short), trimblanklines=True)

	for property in replication_config['Fragments'][fragment]:
		type = replication_config['Fragments'][fragment][property]
		out("""
	void Set%s(const %s In%s) { %s = In%s; }
	%s Get%s() const { return %s; }
		""" % (property, type, property, property, property, type, property, property), trimblanklines=True)

	outl("private:")

	for property in replication_config['Fragments'][fragment]:
		type = replication_config['Fragments'][fragment][property]
		out("""
	UPROPERTY(Transient)
	%s %s = %s;
		""" % (type, property, type_defaults[type]), trimblanklines=True)

	outl("};")
	outl("")

for entity in replication_config['Entities']:
	out("""
USTRUCT()
struct %s FReplicated%sAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()

	""" % (module_macro, entity), trimblanklines=True)

	for data in replication_config['Entities'][entity]['AgentAdditionalReplicatedData']:
		out("""
	const FReplicatedAgent%sData& GetReplicated%sData() const { return %s; }
	FReplicatedAgent%sData& GetReplicated%sDataMutable() { return %s; }
		""" % (data, data, data, data, data, data), trimblanklines=True)

	for fragment in replication_config['Entities'][entity]['Fragments']:
		fragment_short = fragment[5:-8]
		out("""

	const FReplicatedAgent%sData& GetReplicated%sData() const { return %s; }
	FReplicatedAgent%sData& GetReplicated%sDataMutable() { return %s; }
		""" % (fragment_short, fragment_short, fragment_short, fragment_short, fragment_short, fragment_short), trimblanklines=True)

	outl("private:")

	for data in replication_config['Entities'][entity]['AgentAdditionalReplicatedData']:
		out("""
	UPROPERTY()
	FReplicatedAgent%sData %s;
		""" % (data, data), trimblanklines=True)

	for fragment in replication_config['Entities'][entity]['Fragments']:
		fragment_short = fragment[5:-8]
		out("""

	UPROPERTY()
	FReplicatedAgent%sData %s;
		""" % (fragment_short, fragment_short), trimblanklines=True)

	out("""
};

USTRUCT()
struct %s F%sFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()

	F%sFastArrayItem() = default;
	F%sFastArrayItem(const FReplicated%sAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
		: FMassFastArrayItemBase(InHandle)
		, Agent(InAgent)
	{}

	using FReplicatedAgentType = FReplicated%sAgent;

	UPROPERTY()
	FReplicated%sAgent Agent;
};

	""" % (module_macro, entity, entity, entity, entity, entity, entity), trimblanklines=True)

	outl("class F%sClientBubbleHandler;" % (entity))

for fragment in replication_config['Fragments']:
	fragment_short = fragment[5:-8]
	template = Template("""
template<typename AgentArrayItem>
class TMassClientBubble${fragment_short}Handler
{
public:
	TMassClientBubble${fragment_short}Handler(TClientBubbleHandlerBase2<AgentArrayItem>& InOwnerHandler)
		: OwnerHandler(InOwnerHandler)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	void SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMass${fragment_short}Fragment& ${fragment_short}Fragment);

#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery);
	void CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext);
	void ClearFragmentViewsForSpawnQuery();

	void SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgent${fragment_short}Data& Replicated${fragment_short}Data) const;

	static void SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgent${fragment_short}Data& Replicated${fragment_short}Data);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void SetEntityData(FMass${fragment_short}Fragment& ${fragment_short}Fragment, const FReplicatedAgent${fragment_short}Data& Replicated${fragment_short}Data);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
	TArrayView<FMass${fragment_short}Fragment> ${fragment_short}List;

	TClientBubbleHandlerBase2<AgentArrayItem>& OwnerHandler;
};
	""")

	outl(template.substitute(fragment_short=fragment_short))

	template = Template("""
#if UE_REPLICATION_COMPILE_SERVER_CODE
template<typename AgentArrayItem>
void TMassClientBubble${fragment_short}Handler<AgentArrayItem>::SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMass${fragment_short}Fragment& ${fragment_short}Fragment)
{
	check(OwnerHandler.AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = OwnerHandler.AgentLookupArray[Handle.GetIndex()].AgentsIdx;
	bool bMarkDirty = false;

	AgentArrayItem& Item = (*OwnerHandler.Agents)[AgentsIdx];

	checkf(Item.Agent.GetNetID().IsValid(), TEXT("Pos should not be updated on FCrowdFastArrayItem's that have an Invalid ID! First Add the Agent!"));

	FReplicatedAgent${fragment_short}Data& Replicated${fragment_short} = Item.Agent.GetReplicated${fragment_short}DataMutable();

""")

	out(template.substitute(fragment_short=fragment_short))

	for property in replication_config['Fragments'][fragment]:
		type = replication_config['Fragments'][fragment][property]
		template = Template("""
	if (Replicated${fragment_short}.Get${property}() != ${fragment_short}Fragment.${property})
	{
		Replicated${fragment_short}.Set${property}(${fragment_short}Fragment.${property});
		bMarkDirty = true;
	}
		""")

		out(template.substitute(fragment_short=fragment_short, property=property), trimblanklines=True)
		
	out("""

	if (bMarkDirty)
	{
		OwnerHandler.Serializer->MarkItemDirty(Item);
	}
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE
	""", trimblanklines=True)

	out("""

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMass%sFragment>(EMassFragmentAccess::ReadWrite);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext)
{
	%sList = InExecContext.GetMutableFragmentView<FMass%sFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::ClearFragmentViewsForSpawnQuery()
{
	%sList = TArrayView<FMass%sFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgent%sData& Replicated%sData) const
{
	FMass%sFragment& %sFragment = %sList[EntityIdx];

	SetEntityData(%sFragment, Replicated%sData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgent%sData& Replicated%sData)
{
	FMass%sFragment& %sFragment = EntityView.GetFragmentData<FMass%sFragment>();

	SetEntityData(%sFragment, Replicated%sData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubble%sHandler<AgentArrayItem>::SetEntityData(FMass%sFragment& %sFragment, const FReplicatedAgent%sData& Replicated%sData)
{
	""" % tuple([fragment_short]*29), trimblanklines=True)

	for property in replication_config['Fragments'][fragment]:
		out("""
	%sFragment.%s = Replicated%sData.Get%s();
		""" % (fragment_short, property, fragment_short, property), trimblanklines=True)

	out("""
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE
	""", trimblanklines=True)

	out("""

class %s FMassReplicationProcessor%sHandler
{
public:
	static void AddRequirements(FMassEntityQuery& InQuery);
	void CacheFragmentViews(FMassExecutionContext& ExecContext);
	void AddEntity(const int32 EntityIdx, FReplicatedAgent%sData& InOutReplicated%sData) const;

	template<typename AgentArrayItem>
	void ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubble%sHandler<AgentArrayItem>& Bubble%sHandler);

	TArrayView<FMass%sFragment> %sList;
};

template<typename AgentArrayItem>
void FMassReplicationProcessor%sHandler::ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubble%sHandler<AgentArrayItem>& Bubble%sHandler)
{
	const FMass%sFragment& %sFragment = %sList[EntityIdx];
	Bubble%sHandler.SetBubbleData(Handle, %sFragment);
}
	""" % tuple([module_macro] + [fragment_short]*15), trimblanklines=True)

for entity in replication_config['Entities']:
	out("""

class %s F%sClientBubbleHandler : public TClientBubbleHandlerBase2<F%sFastArrayItem>
{
	""" % (module_macro, entity, entity), trimblanklines=True)

	for additional_handler in replication_config['Entities'][entity]['ClientBubbleAdditionalHandlers']:
		out("""
	template<typename T>
	friend class TMassClientBubble%sHandler;
		""" % (additional_handler), trimblanklines=True)

	for fragment in replication_config['Entities'][entity]['Fragments']:
		fragment_short = fragment[5:-8]
		out("""
	template<typename T>
	friend class TMassClientBubble%sHandler;
		""" % (fragment_short), trimblanklines=True)

	outl("""
public:
	typedef TClientBubbleHandlerBase2<F%sFastArrayItem> Super;
	""" % (entity), trimblanklines=True)

	fragments_short = [fragment[5:-8] for fragment in replication_config['Entities'][entity]['Fragments']]
	handlers = replication_config['Entities'][entity]['ClientBubbleAdditionalHandlers'] + fragments_short
	for handler in handlers:
		outl("""
	typedef TMassClientBubble%sHandler<F%sFastArrayItem> FMassClientBubble%sHandler;
		""" % (handler, entity, handler), trimblanklines=True)

	out("""
	F%sClientBubbleHandler()
	""" % (entity), trimblanklines=True)

	outl("		: " + ", ".join(["%sHandler(*this)" % (handler) for handler in handlers]))

	outl("""
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	""", trimblanklines=True)

	for handler in handlers:
		outl("""
	const FMassClientBubble%sHandler& Get%sHandler() const { return %sHandler; }
	FMassClientBubble%sHandler& Get%sHandlerMutable() { return %sHandler; }
		""" % (handler, handler, handler, handler, handler, handler), trimblanklines=True)

	outl("""
#endif // UE_REPLICATION_COMPILE_SERVER_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChange(const TArrayView<int32> ChAMSedIndices, int32 FinalSize) override;

	void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicated%sAgent& Item) const;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
	""" % (entity))

	for handler in handlers:
		out("""
	FMassClientBubble%sHandler %sHandler;
		""" % (handler, handler), trimblanklines=True)

	outl("};\n")

	template = Template("""

USTRUCT()
struct $module_macro F${entity}ClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
	GENERATED_BODY()

	F${entity}ClientBubbleSerializer()
	{
		Bubble.Initialize(Items, *this);
	};

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<F${entity}FastArrayItem, F${entity}ClientBubbleSerializer>(Items, DeltaParams, *this);
	}

public:
	F${entity}ClientBubbleHandler Bubble;

protected:
	UPROPERTY(Transient)
	TArray<F${entity}FastArrayItem> Items;
};

template<>
struct TStructOpsTypeTraits<F${entity}ClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<F${entity}ClientBubbleSerializer>
{
	enum
	{
		WithNetDeltaSerializer = true,
		WithCopy = false,
	};
};

UCLASS()
class $module_macro A${entity}ClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

public:
	A${entity}ClientBubbleInfo(const FObjectInitializer& ObjectInitializer);
	F${entity}ClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, Transient)
	F${entity}ClientBubbleSerializer BubbleSerializer;
};

UCLASS()
class $module_macro U${entity}Replicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	void AddRequirements(FMassEntityQuery& EntityQuery) override;
	void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
	""")
	
	outl(template.substitute(
		entity = entity,
		module_macro = module_macro,
	))

outl("""
UCLASS()
class UMassReplicationBubbleRegistrationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual void PostInitialize() override;
};
""")

write_to_file("MassReplicationHelpersGenerated.h")
