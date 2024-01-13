// THIS IS GENERATED CODE. DO NOT MODIFY.
// REGENERATE WITH: python generate_mass_replication_helpers.py

#pragma once

#include "MassReplicationTypes.h"
#include "MassClientBubbleHandler.h"
#include "MassClientBubbleInfoBase.h"
#include "MassReplicationTransformHandlers.h"
#include "MassEntityView.h"
#include "MassReplicationProcessor.h"

#include "MassSample/Unit/MSUnitFragments.h"

#include "MassReplicationHelpersGenerated.generated.h"

template<typename AgentArrayItem>
class TClientBubbleHandlerBase2 : public TClientBubbleHandlerBase<AgentArrayItem>
{
public:

	template<typename T>
	friend class TMassClientBubbleHealthHandler;

	template<typename T>
	friend class TMassClientBubbleLifetimeHandler;

};

USTRUCT()
struct MASSSAMPLE_API FReplicatedAgentHealthData
{
	GENERATED_BODY()

	FReplicatedAgentHealthData() = default;

	void SetValue(const int32 InValue) { Value = InValue; }
	int32 GetValue() const { return Value; }
	void SetbIsBleeding(const bool InbIsBleeding) { bIsBleeding = InbIsBleeding; }
	bool GetbIsBleeding() const { return bIsBleeding; }
private:
	UPROPERTY(Transient)
	int32 Value = 0;
	UPROPERTY(Transient)
	bool bIsBleeding = false;
};

USTRUCT()
struct MASSSAMPLE_API FReplicatedAgentLifetimeData
{
	GENERATED_BODY()

	FReplicatedAgentLifetimeData() = default;

	void SetValue(const float InValue) { Value = InValue; }
	float GetValue() const { return Value; }
private:
	UPROPERTY(Transient)
	float Value = 0.f;
};

USTRUCT()
struct MASSSAMPLE_API FReplicatedMSUnitAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()

	const FReplicatedAgentPositionYawData& GetReplicatedPositionYawData() const { return PositionYaw; }
	FReplicatedAgentPositionYawData& GetReplicatedPositionYawDataMutable() { return PositionYaw; }

	const FReplicatedAgentHealthData& GetReplicatedHealthData() const { return Health; }
	FReplicatedAgentHealthData& GetReplicatedHealthDataMutable() { return Health; }

	const FReplicatedAgentLifetimeData& GetReplicatedLifetimeData() const { return Lifetime; }
	FReplicatedAgentLifetimeData& GetReplicatedLifetimeDataMutable() { return Lifetime; }
private:
	UPROPERTY()
	FReplicatedAgentPositionYawData PositionYaw;

	UPROPERTY()
	FReplicatedAgentHealthData Health;

	UPROPERTY()
	FReplicatedAgentLifetimeData Lifetime;
};

USTRUCT()
struct MASSSAMPLE_API FMSUnitFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()

	FMSUnitFastArrayItem() = default;
	FMSUnitFastArrayItem(const FReplicatedMSUnitAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
		: FMassFastArrayItemBase(InHandle)
		, Agent(InAgent)
	{}

	using FReplicatedAgentType = FReplicatedMSUnitAgent;

	UPROPERTY()
	FReplicatedMSUnitAgent Agent;
};

class FMSUnitClientBubbleHandler;

template<typename AgentArrayItem>
class TMassClientBubbleHealthHandler
{
public:
	TMassClientBubbleHealthHandler(TClientBubbleHandlerBase2<AgentArrayItem>& InOwnerHandler)
		: OwnerHandler(InOwnerHandler)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	void SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMassHealthFragment& HealthFragment);

#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery);
	void CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext);
	void ClearFragmentViewsForSpawnQuery();

	void SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentHealthData& ReplicatedHealthData) const;

	static void SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentHealthData& ReplicatedHealthData);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void SetEntityData(FMassHealthFragment& HealthFragment, const FReplicatedAgentHealthData& ReplicatedHealthData);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
	TArrayView<FMassHealthFragment> HealthList;

	TClientBubbleHandlerBase2<AgentArrayItem>& OwnerHandler;
};
	

#if UE_REPLICATION_COMPILE_SERVER_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMassHealthFragment& HealthFragment)
{
	check(OwnerHandler.AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = OwnerHandler.AgentLookupArray[Handle.GetIndex()].AgentsIdx;
	bool bMarkDirty = false;

	AgentArrayItem& Item = (*OwnerHandler.Agents)[AgentsIdx];

	checkf(Item.Agent.GetNetID().IsValid(), TEXT("Pos should not be updated on FCrowdFastArrayItem's that have an Invalid ID! First Add the Agent!"));

	FReplicatedAgentHealthData& ReplicatedHealth = Item.Agent.GetReplicatedHealthDataMutable();

	if (ReplicatedHealth.GetValue() != HealthFragment.Value)
	{
		ReplicatedHealth.SetValue(HealthFragment.Value);
		bMarkDirty = true;
	}
	if (ReplicatedHealth.GetbIsBleeding() != HealthFragment.bIsBleeding)
	{
		ReplicatedHealth.SetbIsBleeding(HealthFragment.bIsBleeding);
		bMarkDirty = true;
	}

	if (bMarkDirty)
	{
		OwnerHandler.Serializer->MarkItemDirty(Item);
	}
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMassHealthFragment>(EMassFragmentAccess::ReadWrite);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext)
{
	HealthList = InExecContext.GetMutableFragmentView<FMassHealthFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::ClearFragmentViewsForSpawnQuery()
{
	HealthList = TArrayView<FMassHealthFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentHealthData& ReplicatedHealthData) const
{
	FMassHealthFragment& HealthFragment = HealthList[EntityIdx];

	SetEntityData(HealthFragment, ReplicatedHealthData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentHealthData& ReplicatedHealthData)
{
	FMassHealthFragment& HealthFragment = EntityView.GetFragmentData<FMassHealthFragment>();

	SetEntityData(HealthFragment, ReplicatedHealthData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetEntityData(FMassHealthFragment& HealthFragment, const FReplicatedAgentHealthData& ReplicatedHealthData)
{
	HealthFragment.Value = ReplicatedHealthData.GetValue();
	HealthFragment.bIsBleeding = ReplicatedHealthData.GetbIsBleeding();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

class MASSSAMPLE_API FMassReplicationProcessorHealthHandler
{
public:
	static void AddRequirements(FMassEntityQuery& InQuery);
	void CacheFragmentViews(FMassExecutionContext& ExecContext);
	void AddEntity(const int32 EntityIdx, FReplicatedAgentHealthData& InOutReplicatedHealthData) const;

	template<typename AgentArrayItem>
	void ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleHealthHandler<AgentArrayItem>& BubbleHealthHandler);

	TArrayView<FMassHealthFragment> HealthList;
};

template<typename AgentArrayItem>
void FMassReplicationProcessorHealthHandler::ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleHealthHandler<AgentArrayItem>& BubbleHealthHandler)
{
	const FMassHealthFragment& HealthFragment = HealthList[EntityIdx];
	BubbleHealthHandler.SetBubbleData(Handle, HealthFragment);
}

template<typename AgentArrayItem>
class TMassClientBubbleLifetimeHandler
{
public:
	TMassClientBubbleLifetimeHandler(TClientBubbleHandlerBase2<AgentArrayItem>& InOwnerHandler)
		: OwnerHandler(InOwnerHandler)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	void SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMassLifetimeFragment& LifetimeFragment);

#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery);
	void CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext);
	void ClearFragmentViewsForSpawnQuery();

	void SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData) const;

	static void SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void SetEntityData(FMassLifetimeFragment& LifetimeFragment, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
	TArrayView<FMassLifetimeFragment> LifetimeList;

	TClientBubbleHandlerBase2<AgentArrayItem>& OwnerHandler;
};
	

#if UE_REPLICATION_COMPILE_SERVER_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::SetBubbleData(const FMassReplicatedAgentHandle Handle, const FMassLifetimeFragment& LifetimeFragment)
{
	check(OwnerHandler.AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = OwnerHandler.AgentLookupArray[Handle.GetIndex()].AgentsIdx;
	bool bMarkDirty = false;

	AgentArrayItem& Item = (*OwnerHandler.Agents)[AgentsIdx];

	checkf(Item.Agent.GetNetID().IsValid(), TEXT("Pos should not be updated on FCrowdFastArrayItem's that have an Invalid ID! First Add the Agent!"));

	FReplicatedAgentLifetimeData& ReplicatedLifetime = Item.Agent.GetReplicatedLifetimeDataMutable();

	if (FMath::Abs(ReplicatedLifetime.GetValue() - LifetimeFragment.Value) > 0.1f)
	{
		ReplicatedLifetime.SetValue(LifetimeFragment.Value);
		bMarkDirty = true;
	}

	if (bMarkDirty)
	{
		OwnerHandler.Serializer->MarkItemDirty(Item);
	}
}
#endif //UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery)
{
	InQuery.AddRequirement<FMassLifetimeFragment>(EMassFragmentAccess::ReadWrite);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext)
{
	LifetimeList = InExecContext.GetMutableFragmentView<FMassLifetimeFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::ClearFragmentViewsForSpawnQuery()
{
	LifetimeList = TArrayView<FMassLifetimeFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData) const
{
	FMassLifetimeFragment& LifetimeFragment = LifetimeList[EntityIdx];

	SetEntityData(LifetimeFragment, ReplicatedLifetimeData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData)
{
	FMassLifetimeFragment& LifetimeFragment = EntityView.GetFragmentData<FMassLifetimeFragment>();

	SetEntityData(LifetimeFragment, ReplicatedLifetimeData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleLifetimeHandler<AgentArrayItem>::SetEntityData(FMassLifetimeFragment& LifetimeFragment, const FReplicatedAgentLifetimeData& ReplicatedLifetimeData)
{
	LifetimeFragment.Value = ReplicatedLifetimeData.GetValue();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

class MASSSAMPLE_API FMassReplicationProcessorLifetimeHandler
{
public:
	static void AddRequirements(FMassEntityQuery& InQuery);
	void CacheFragmentViews(FMassExecutionContext& ExecContext);
	void AddEntity(const int32 EntityIdx, FReplicatedAgentLifetimeData& InOutReplicatedLifetimeData) const;

	template<typename AgentArrayItem>
	void ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleLifetimeHandler<AgentArrayItem>& BubbleLifetimeHandler);

	TArrayView<FMassLifetimeFragment> LifetimeList;
};

template<typename AgentArrayItem>
void FMassReplicationProcessorLifetimeHandler::ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleLifetimeHandler<AgentArrayItem>& BubbleLifetimeHandler)
{
	const FMassLifetimeFragment& LifetimeFragment = LifetimeList[EntityIdx];
	BubbleLifetimeHandler.SetBubbleData(Handle, LifetimeFragment);
}

class MASSSAMPLE_API FMSUnitClientBubbleHandler : public TClientBubbleHandlerBase2<FMSUnitFastArrayItem>
{
	template<typename T>
	friend class TMassClientBubbleTransformHandler;
	template<typename T>
	friend class TMassClientBubbleHealthHandler;
	template<typename T>
	friend class TMassClientBubbleLifetimeHandler;
public:
	typedef TClientBubbleHandlerBase2<FMSUnitFastArrayItem> Super;

	typedef TMassClientBubbleTransformHandler<FMSUnitFastArrayItem> FMassClientBubbleTransformHandler;

	typedef TMassClientBubbleHealthHandler<FMSUnitFastArrayItem> FMassClientBubbleHealthHandler;

	typedef TMassClientBubbleLifetimeHandler<FMSUnitFastArrayItem> FMassClientBubbleLifetimeHandler;

	FMSUnitClientBubbleHandler()
		: TransformHandler(*this), HealthHandler(*this), LifetimeHandler(*this)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE

	const FMassClientBubbleTransformHandler& GetTransformHandler() const { return TransformHandler; }
	FMassClientBubbleTransformHandler& GetTransformHandlerMutable() { return TransformHandler; }

	const FMassClientBubbleHealthHandler& GetHealthHandler() const { return HealthHandler; }
	FMassClientBubbleHealthHandler& GetHealthHandlerMutable() { return HealthHandler; }

	const FMassClientBubbleLifetimeHandler& GetLifetimeHandler() const { return LifetimeHandler; }
	FMassClientBubbleLifetimeHandler& GetLifetimeHandlerMutable() { return LifetimeHandler; }


#endif // UE_REPLICATION_COMPILE_SERVER_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChange(const TArrayView<int32> ChAMSedIndices, int32 FinalSize) override;

	void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicatedMSUnitAgent& Item) const;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
	
	FMassClientBubbleTransformHandler TransformHandler;
	FMassClientBubbleHealthHandler HealthHandler;
	FMassClientBubbleLifetimeHandler LifetimeHandler;
};



USTRUCT()
struct MASSSAMPLE_API FMSUnitClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
	GENERATED_BODY()

	FMSUnitClientBubbleSerializer()
	{
		Bubble.Initialize(Items, *this);
	};

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMSUnitFastArrayItem, FMSUnitClientBubbleSerializer>(Items, DeltaParams, *this);
	}

public:
	FMSUnitClientBubbleHandler Bubble;

protected:
	UPROPERTY(Transient)
	TArray<FMSUnitFastArrayItem> Items;
};

template<>
struct TStructOpsTypeTraits<FMSUnitClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<FMSUnitClientBubbleSerializer>
{
	enum
	{
		WithNetDeltaSerializer = true,
		WithCopy = false,
	};
};

UCLASS()
class MASSSAMPLE_API AMSUnitClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

public:
	AMSUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer);
	FMSUnitClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, Transient)
	FMSUnitClientBubbleSerializer BubbleSerializer;
};

UCLASS()
class MASSSAMPLE_API UMSUnitReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	void AddRequirements(FMassEntityQuery& EntityQuery) override;
	void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
	

UCLASS()
class UMassReplicationBubbleRegistrationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual void PostInitialize() override;
};

