#pragma once

#include "MassSpawnerSubsystem.h"
#include "MassSpawnLocationProcessor.h"
#include "MassAgentComponent.h"

#include "MSEntitySystem.generated.h"

FMassEntityHandle GetMassEntityHandle(const AActor* Actor);

template<typename T>
static T* GetFragmentForActor(const AActor* Actor)
{
	check(Actor);
	if (!Actor)
	{
		return nullptr;
	}
	const UWorld* World = Actor->GetWorld();
	const FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(World);
	if (!EntityManager)
	{
		return nullptr;
	}

	const FMassEntityHandle& Entity = GetMassEntityHandle(Actor);
	if (!Entity.IsValid() || !EntityManager->IsEntityValid(Entity))
	{
		return nullptr;
	}

	return EntityManager->GetFragmentDataPtr<T>(Entity);
}

class UMassEntityConfigAsset;
UCLASS()
class UMSEntitySystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "MassSample")
	void Spawn(UMassEntityConfigAsset* EntityConfig, const FTransform& Transform);

	UFUNCTION(BlueprintPure, Category = "MassSample")
	static int32 GetHealthForActor(AActor* Actor);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PostInitialize() override;

private:
	TSharedPtr<FMassEntityManager> EntityManager;
};
