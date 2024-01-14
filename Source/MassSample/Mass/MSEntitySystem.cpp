#include "MSEntitySystem.h"

#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "MassSample/MassUpdateEntitiesProcessor.h"
#include "MassSample/MSAssetManager.h"
#include "MassSample/Data/MSGameData.h"
#include "MassSample/Unit/MSUnitFragments.h"

namespace MSEntitySystem::Tweakables
{
	int32 SpawnCount = 10000;

	FAutoConsoleVariableRef CVars[] =
	{
		FAutoConsoleVariableRef(TEXT("ms.SpawnCount"), SpawnCount, TEXT("Set number of rabbits to spawn"), ECVF_Cheat),
	};
}

/*static*/ FMassEntityHandle GetMassEntityHandle(const AActor* Actor)
{
	if (Actor == nullptr)
	{
		return FMassEntityHandle();
	}

	const UMassAgentComponent* AgentComponent = Cast<UMassAgentComponent>(Actor->GetComponentByClass(UMassAgentComponent::StaticClass()));
	if (!AgentComponent)
	{
		return FMassEntityHandle();
	}

	return AgentComponent->GetEntityHandle();
}

void UMSEntitySystem::Spawn(UMassEntityConfigAsset* EntityConfig, const FTransform& Transform)
{
	const FMassEntityTemplate& EntityTemplate = EntityConfig->GetOrCreateEntityTemplate(*GetWorld());

	TArray<FMassEntityHandle> Entities;
	const auto CreationContext = EntityManager->BatchCreateEntities(EntityTemplate.GetArchetype(), EntityTemplate.GetSharedFragmentValues(), MSEntitySystem::Tweakables::SpawnCount, Entities);
	const TConstArrayView<FInstancedStruct> FragmentInstances = EntityTemplate.GetInitialFragmentValues();
	EntityManager->BatchSetEntityFragmentsValues(CreationContext->GetEntityCollection(), FragmentInstances);

	const int SqrtCount = FMath::Sqrt(static_cast<float>(MSEntitySystem::Tweakables::SpawnCount));

	int j = 0;
	static constexpr float EntitySpacing = 1000.f;
	const float GridHalfSize = (SqrtCount * EntitySpacing) / 2.f;
	for (int i = 0; i < Entities.Num(); ++i)
	{
		FMassEntityView EntityView(EntityTemplate.GetArchetype(), Entities[i]);
		auto& TransformData = EntityView.GetFragmentData<FTransformFragment>();

		FVector RabbitLocation = Transform.GetLocation();
		RabbitLocation.X = (i % SqrtCount) * EntitySpacing - GridHalfSize;
		if (i % SqrtCount == 0) ++j;
		RabbitLocation.Y = j * EntitySpacing - GridHalfSize;
		TransformData.SetTransform(FTransform(RabbitLocation));
	}
}

void UMSEntitySystem::Initialize(FSubsystemCollectionBase& Collection)
{
	auto* MassSubsystem = Collection.InitializeDependency<UMassEntitySubsystem>();
	check(MassSubsystem);

	EntityManager = MassSubsystem->GetMutableEntityManager().AsShared();
}

void UMSEntitySystem::Deinitialize()
{
	EntityManager.Reset();
}

void UMSEntitySystem::PostInitialize()
{
	const auto* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(GetWorld());
	check(SpawnerSubsystem);

	if (UMSAssetManager::Get()->GameData)
	{
		const auto& EntityTemplate = UMSAssetManager::Get()->GameData->UnitEntityConfig->GetOrCreateEntityTemplate(*GetWorld());
		ensure(EntityTemplate.IsValid());
	}
}

/*static*/ int32 UMSEntitySystem::GetHealthForActor(const AActor* Actor)
{
	if (const FMassHealthFragment* HealthFragment = GetFragmentForActor<FMassHealthFragment>(Actor))
	{
		return HealthFragment->Value;
	}

	return -1;
}

/*static*/ bool UMSEntitySystem::GetIsBleedingForActor(const AActor* Actor)
{
	if (const FMassHealthFragment* HealthFragment = GetFragmentForActor<FMassHealthFragment>(Actor))
	{
		return HealthFragment->bIsBleeding;
	}

	return false;
}

/*static*/ float UMSEntitySystem::GetLifetimeForActor(const AActor* Actor)
{
	if (const FMassLifetimeFragment* LifetimeFragment = GetFragmentForActor<FMassLifetimeFragment>(Actor))
	{
		return LifetimeFragment->Value;
	}

	return 0.f;
}

/*static*/ void UMSEntitySystem::GetEntityForActor(const AActor* Actor, int32& OutEntityIndex, int32& OutEntitySerialNumber)
{
	const FMassEntityHandle EntityHandle = GetMassEntityHandle(Actor);
	if (!EntityHandle.IsValid())
	{
		return;
	}

	OutEntityIndex = EntityHandle.Index;
	OutEntitySerialNumber = EntityHandle.SerialNumber;
}

/*static*/ int32 UMSEntitySystem::GetNetIDForActorEntity(const AActor* Actor)
{
	if (const FMassNetworkIDFragment* NetworkIDFragment = GetFragmentForActor<FMassNetworkIDFragment>(Actor))
	{
		const uint32 NetID = NetworkIDFragment->NetID.GetValue();
		const UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(Actor->GetWorld());
		const FMassEntityHandle Entity = ReplicationSubsystem->FindEntity(FMassNetworkID(static_cast<uint32>(NetID)));
		UE_LOG(LogTemp, Warning, TEXT("Actor %s has Entity %s and NetID [%d]"), *Actor->GetName(), *Entity.DebugGetDescription(), NetID);

		return static_cast<int32>(NetID);
	}

	return 0.f;
}

/*static*/ void UMSEntitySystem::MoveEntity(const UObject* WorldContextObject, const int32 EntityNetID, const FVector& Delta)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	const UMassReplicationNetworkIDEntityMapSubsystem* NetworkIDEntityMapSubsystem = UWorld::GetSubsystem<UMassReplicationNetworkIDEntityMapSubsystem>(World);
	const FMassEntityHandle* Entity = NetworkIDEntityMapSubsystem->NetworkIDEntityMap.Find(FMassNetworkID(static_cast<uint32>(EntityNetID)));

	if (!Entity || !Entity->IsValid())
	{
		return;
	}

	const FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(World);
	if (!EntityManager)
	{
		return;
	}

	if (FMassHealthFragment* HealthFragment = EntityManager->GetFragmentDataPtr<FMassHealthFragment>(*Entity))
	{
		HealthFragment->Value -= 1;
	}

	if (FTransformFragment* TransformFragment = EntityManager->GetFragmentDataPtr<FTransformFragment>(*Entity))
	{
		TransformFragment->GetMutableTransform().AddToTranslation(Delta);
	}

	//UMassReplicationRpcSubsystem* RpcSubsystem = World->GetSubsystem<UMassReplicationRpcSubsystem>();

	//FMassReplicationRpc Rpc;
	//Rpc.Type = EMassReplicationRpcType::Move;
	//Rpc.Delta = Delta;
	//RpcSubsystem->Queue.Add(static_cast<uint32>(EntityNetID), Rpc);

	//UE_LOG(LogTemp, Warning, TEXT("Entity with NetID [%d] queued RPC with delta: %s"), EntityNetID, *Delta.ToCompactString());
}

/*static*/ void UMSEntitySystem::ToggleUpdateEntityProcessor(const UObject* WorldContextObject, const int32 EntityIndex, const int32 EntitySerialNumber)
{
	UE_LOG(LogTemp, Error, TEXT("ToggleUpdateEntityProcessor not yet implemented"));

	//const FMassEntityHandle Entity(EntityIndex, EntitySerialNumber);
	//const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);

	//UMassReplicationRpcSubsystem* RpcSubsystem = World->GetSubsystem<UMassReplicationRpcSubsystem>();

	//FMassReplicationRpc Rpc;
	//Rpc.Type = EMassReplicationRpcType::ToggleUpdateEntitiesProcessor;
	//RpcSubsystem->Queue.Add(Entity, Rpc);

	//UE_LOG(LogTemp, Warning, TEXT("Entity [%s] queued generic RPC"), *Entity.DebugGetDescription());
}
