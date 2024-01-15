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
	static int32 GetHealthForActor(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "MassSample")
	static bool GetIsBleedingForActor(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "MassSample")
	static float GetLifetimeForActor(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "MassSample", meta=(DefaultToSelf = "Actor"))
	static void GetEntityForActor(const AActor* Actor, int32& OutEntityIndex, int32& OutEntitySerialNumber);

	// Although NetIDs are uint32, we use int32 here because Blueprint doesn't support uint32.
	UFUNCTION(BlueprintPure, Category = "MassSample", meta=(DefaultToSelf = "Actor"))
	static int32 GetNetIDForActorEntity(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "MassSample", meta = (WorldContext="WorldContextObject"))
	static void MoveEntity(const UObject* WorldContextObject, const int32 EntityNetID, const FVector& Delta);

	UFUNCTION(BlueprintCallable, Category = "MassSample", meta = (WorldContext="WorldContextObject"))
	static void ToggleUpdateEntityProcessor(const UObject* WorldContextObject, int32 EntityIndex, int32 EntitySerialNumber);

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PostInitialize() override;

private:
	TSharedPtr<FMassEntityManager> EntityManager;
};
