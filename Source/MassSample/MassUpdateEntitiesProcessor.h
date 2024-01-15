#pragma once

#include "MassCommonTypes.h"
#include "MassObserverProcessor.h"
#include "MassProcessor.h"

#include "MassUpdateEntitiesProcessor.generated.h"

UCLASS()
class MASSSAMPLE_API UMassUpdateEntitiesProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassUpdateEntitiesProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	FMassEntityQuery EntityQuery;
	double LastWorldTimeSecondsWhenRan = -1;
	bool bIsEnabled = false;
};

UCLASS()
class UMassReplicationNetworkIDEntityMapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	TMap<FMassNetworkID, FMassEntityHandle> NetworkIDEntityMap;
};

UCLASS()
class MASSSAMPLE_API UMassNetworkIDEntityMapInitializer : public UMassObserverProcessor
{
	GENERATED_BODY()
public:
	UMassNetworkIDEntityMapInitializer();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

UCLASS()
class MASSSAMPLE_API UMSBPFunctionLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

  UFUNCTION(BlueprintPure, Category = "MassSample", meta = (WorldContext = "WorldContextObject") )
  static AActor* GetActorForEntityWithNetID(const UObject* WorldContextObject, const int32 EntityNetID);
};