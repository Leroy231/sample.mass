#include "MSEntitySystem.h"

#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntitySubsystem.h"
#include "MassEntityView.h"
#include "MassReplicationSubsystem.h"
#include "MassSample/MSAssetManager.h"
#include "MassSample/Data/MSGameData.h"
#include "MassSample/Unit/MSUnitFragments.h"
#include "Replication/MSMassReplicationHelpers.h"

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
	auto* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

	ReplicationSubsystem->RegisterBubbleInfoClass(AMSUnitClientBubbleInfo::StaticClass());

	const auto* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(GetWorld());
	check(SpawnerSubsystem);

	if (UMSAssetManager::Get()->GameData)
	{
		const auto& EntityTemplate = UMSAssetManager::Get()->GameData->UnitEntityConfig->GetOrCreateEntityTemplate(*GetWorld());
		ensure(EntityTemplate.IsValid());
	}
}

int32 UMSEntitySystem::GetHealthForActor(const AActor* Actor)
{
	if (const FMassHealthFragment* HealthFragment = GetFragmentForActor<FMassHealthFragment>(Actor))
	{
		return HealthFragment->Health;
	}

	return -1;
}
