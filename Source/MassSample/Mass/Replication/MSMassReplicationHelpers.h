#pragma once

#include "MassReplicationTypes.h"
#include "MassClientBubbleHandler.h"
#include "MassClientBubbleInfoBase.h"
#include "MassReplicationTransformHandlers.h"
#include "MassEntityView.h"
#include "MassSample/Unit/MSUnitFragments.h"

#include "MSMassReplicationHelpers.generated.h"

USTRUCT()
struct MASSSAMPLE_API FReplicatedAgentHealthData
{
	GENERATED_BODY()

	FReplicatedAgentHealthData() = default;

	void SetHealth(const int32 InHealth) { Health = InHealth; }
	int32 GetHealth() const { return Health; }

private:
	UPROPERTY(Transient)
	int32 Health = -1;
};

/** The data that is replicated specific to each Crowd agent */
USTRUCT()
struct MASSSAMPLE_API FMSReplicatedUnitAgent : public FReplicatedAgentBase
{
    GENERATED_BODY()

    const FReplicatedAgentPositionYawData& GetReplicatedPositionYawData() const { return PositionYaw; }
    FReplicatedAgentPositionYawData& GetReplicatedPositionYawDataMutable() { return PositionYaw; }

    const FReplicatedAgentHealthData& GetReplicatedHealthData() const { return Health; }
    FReplicatedAgentHealthData& GetReplicatedHealthDataMutable() { return Health; }

private:
    UPROPERTY()
    FReplicatedAgentPositionYawData PositionYaw; // replicated data

    UPROPERTY()
    FReplicatedAgentHealthData Health; // replicated data
};

/** Fast array item for efficient agent replication. Remember to make this dirty if any FReplicatedCrowdAgent member variables are modified */
USTRUCT()
struct MASSSAMPLE_API FMSUnitFastArrayItem : public FMassFastArrayItemBase
{
    GENERATED_BODY()

    FMSUnitFastArrayItem() = default;
    FMSUnitFastArrayItem(const FMSReplicatedUnitAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
        : FMassFastArrayItemBase(InHandle)
        , Agent(InAgent)
    {}

    /** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */
    using FReplicatedAgentType = FMSReplicatedUnitAgent;

    UPROPERTY()
    FMSReplicatedUnitAgent Agent;
};

class FMSUnitClientBubbleHandler;

//////////////////////////////////////////////////////////////////////////// TMassClientBubbleHealthHandler ////////////////////////////////////////////////////////////////////////////
/**
 * To replicate Health make a member variable of this class in your TClientBubbleHandlerBase derived class.
 * It is meant to have access to the protected data declared there.
 */
template<typename AgentArrayItem>
class TMassClientBubbleHealthHandler
{
public:
	TMassClientBubbleHealthHandler(FMSUnitClientBubbleHandler& InOwnerHandler)
		: OwnerHandler(InOwnerHandler)
	{}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	/** Sets the health data in the client bubble on the server */
	void SetBubbleHealth(const FMassReplicatedAgentHandle Handle, const int32 Health);

#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
	/**
	 * When entities are spawned in Mass by the replication system on the client, a spawn query is used to set the data on the spawned entities.
	 * The following functions are used to configure the query and then set the health data.
	 */
	static void AddRequirementsForSpawnQuery(FMassEntityQuery& InQuery);
	void CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext);
	void ClearFragmentViewsForSpawnQuery();

	void SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentHealthData& ReplicatedHealthData) const;

	/** Call this when an Entity that has already been spawned is modified on the client */
	static void SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentHealthData& ReplicatedHealthData);

	// We could easily add support replicating FReplicatedAgentTransformData here if required
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	static void SetEntityData(FMSHealthFragment& HealthFragment, const FReplicatedAgentHealthData& ReplicatedHealthData);
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

protected:
	TArrayView<FMSHealthFragment> HealthList;

	FMSUnitClientBubbleHandler& OwnerHandler;
};

#if UE_REPLICATION_COMPILE_SERVER_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetBubbleHealth(const FMassReplicatedAgentHandle Handle, const int32 Health)
{
	check(OwnerHandler.AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = OwnerHandler.AgentLookupArray[Handle.GetIndex()].AgentsIdx;
	bool bMarkDirty = false;

	AgentArrayItem& Item = (*OwnerHandler.Agents)[AgentsIdx];

	checkf(Item.Agent.GetNetID().IsValid(), TEXT("Pos should not be updated on FCrowdFastArrayItem's that have an Invalid ID! First Add the Agent!"));

	FReplicatedAgentHealthData& ReplicatedHealth = Item.Agent.GetReplicatedHealthDataMutable();

	if (ReplicatedHealth.GetHealth() != Health)
	{
		ReplicatedHealth.SetHealth(Health);
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
	InQuery.AddRequirement<FMSHealthFragment>(EMassFragmentAccess::ReadWrite);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::CacheFragmentViewsForSpawnQuery(FMassExecutionContext& InExecContext)
{
	HealthList = InExecContext.GetMutableFragmentView<FMSHealthFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::ClearFragmentViewsForSpawnQuery()
{
	HealthList = TArrayView<FMSHealthFragment>();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetSpawnedEntityData(const int32 EntityIdx, const FReplicatedAgentHealthData& ReplicatedHealthData) const
{
	FMSHealthFragment& HealthFragment = HealthList[EntityIdx];

	SetEntityData(HealthFragment, ReplicatedHealthData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetModifiedEntityData(const FMassEntityView& EntityView, const FReplicatedAgentHealthData& ReplicatedHealthData)
{
	FMSHealthFragment& HealthFragment = EntityView.GetFragmentData<FMSHealthFragment>();

	SetEntityData(HealthFragment, ReplicatedHealthData);
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
template<typename AgentArrayItem>
void TMassClientBubbleHealthHandler<AgentArrayItem>::SetEntityData(FMSHealthFragment& HealthFragment, const FReplicatedAgentHealthData& ReplicatedHealthData)
{
	HealthFragment.Health = ReplicatedHealthData.GetHealth();
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

//////////////////////////////////////////////////////////////////////////// FMassReplicationProcessorHealthHandler ////////////////////////////////////////////////////////////////////////////
/**
 * Used to replicate health by your UMassReplicationProcessorBase derived class. This class should only get used on the server.
 * @todo add #if UE_REPLICATION_COMPILE_SERVER_CODE
 */
class MASSSAMPLE_API FMassReplicationProcessorHealthHandler
{
public:
	static void AddRequirements(FMassEntityQuery& InQuery);

	void CacheFragmentViews(FMassExecutionContext& ExecContext);

	void AddEntity(const int32 EntityIdx, FReplicatedAgentHealthData& InOutReplicatedHealthData) const;

	template<typename AgentArrayItem>
	void ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleHealthHandler<AgentArrayItem>& BubbleHealthHandler);

	TArrayView<FMSHealthFragment> HealthList;
};

template<typename AgentArrayItem>
void FMassReplicationProcessorHealthHandler::ModifyEntity(const FMassReplicatedAgentHandle Handle, const int32 EntityIdx, TMassClientBubbleHealthHandler<AgentArrayItem>& BubbleHealthHandler)
{
	const FMSHealthFragment& HealthFragment = HealthList[EntityIdx];

	BubbleHealthHandler.SetBubbleHealth(Handle, HealthFragment.Health);
}

class MASSSAMPLE_API FMSUnitClientBubbleHandler : public TClientBubbleHandlerBase<FMSUnitFastArrayItem>
{
	template<typename T>
	friend class TMassClientBubbleTransformHandler;
	template<typename T>
	friend class TMassClientBubbleHealthHandler;
public:
    typedef TClientBubbleHandlerBase<FMSUnitFastArrayItem> Super;
    typedef TMassClientBubbleTransformHandler<FMSUnitFastArrayItem> FMassClientBubbleTransformHandler;
    typedef TMassClientBubbleHealthHandler<FMSUnitFastArrayItem> FMassClientBubbleHealthHandler;

    FMSUnitClientBubbleHandler()
        : TransformHandler(*this), HealthHandler(*this)
    {}

#if UE_REPLICATION_COMPILE_SERVER_CODE
    const FMassClientBubbleTransformHandler& GetTransformHandler() const { return TransformHandler; }
    FMassClientBubbleTransformHandler& GetTransformHandlerMutable() { return TransformHandler; }

	const FMassClientBubbleHealthHandler& GetHealthHandler() const { return HealthHandler; }
    FMassClientBubbleHealthHandler& GetHealthHandlerMutable() { return HealthHandler; }
#endif // UE_REPLICATION_COMPILE_SERVER_CODE


protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
    virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
    virtual void PostReplicatedChange(const TArrayView<int32> ChAMSedIndices, int32 FinalSize) override;

    void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FMSReplicatedUnitAgent& Item) const;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE


    FMassClientBubbleTransformHandler TransformHandler;
    FMassClientBubbleHealthHandler HealthHandler;
};

/** Mass client bubble, there will be one of these per client and it will handle replicating the fast array of Agents between the server and clients */
USTRUCT()
struct MASSSAMPLE_API FMSUnitClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
    GENERATED_BODY()

    FMSUnitClientBubbleSerializer()
    {
        Bubble.Initialize(Units, *this);
    };

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMSUnitFastArrayItem, FMSUnitClientBubbleSerializer>(Units, DeltaParams, *this);
    }

public:
    FMSUnitClientBubbleHandler Bubble;

protected:
    /** Fast Array of Agents for efficient replication. Maintained as a freelist on the server, to keep index consistency as indexes are used as Handles into the Array
     *  Note array order is not guaranteed between server and client so handles will not be consistent between them, FMassNetworkID will be.
     */
    UPROPERTY(Transient)
    TArray<FMSUnitFastArrayItem> Units;
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

/**
 *  This class will allow us to replicate Mass data based on the fidelity required for each player controller. There is one AMassReplicationActor per PlayerController and
 *  which is also its owner.
 */
UCLASS()
class MASSSAMPLE_API AMSUnitClientBubbleInfo : public AMassClientBubbleInfoBase
{
    GENERATED_BODY()

public:
    AMSUnitClientBubbleInfo(const FObjectInitializer& ObjectInitializer);

    FMSUnitClientBubbleSerializer& GetUnitSerializer() { return UnitSerializer; }

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(Replicated, Transient)
    FMSUnitClientBubbleSerializer UnitSerializer;
};