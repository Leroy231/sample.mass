// Copyright (c) Haywire Interactive, LLC. All Rights Reserved.
// Adapted from GameplayDebuggerCategory_Mass.h.

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"
#include "Containers/BinaryHeap.h"

struct FMassRepresentationLODFragment;
struct FMassRepresentationFragment;
struct FMassVelocityFragment;
struct FMassForceFragment;
struct FMassEntityHandle;
struct FMassExecutionContext;
struct FMassMoveTargetFragment;
struct FMassEntityManager;
class APlayerController;
class AActor;

class FGameplayDebuggerCategory_ProjectM : public FGameplayDebuggerCategory
{
public:
  FGameplayDebuggerCategory_ProjectM();
  void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
  void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

  static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	void DrawTargetEntityLocations(const TArray<FVector>& TargetEntityLocations, const FColor& Color, const FVector& EntityLocation, const bool bIncludeDistance = false);
	void CollectDataForEntities(const APlayerController* OwnerPC, const FVector& ViewLocation, const FVector& ViewDirection);
	void CollectDataForEntity(const FTransform& Transform, const float MinViewDirDot, const FVector& ViewLocation, const FVector& ViewDirection, const float MaxViewDistance, const FMassMoveTargetFragment* MoveTargetFragment, const float AgentRadius, const FMassEntityHandle& Entity, const FMassForceFragment& ForceFragment, const FMassVelocityFragment& VelocityFragment, const FMassRepresentationFragment& RepresentationFragment, const FMassRepresentationLODFragment& RepresentationLODFragment, const FVector& FirstViewerLocation);

	void OnToggleShowEntityLocations() { bShowEntityLocations = !bShowEntityLocations; }
	void OnToggleShowEntityAgentRadii() { bShowEntityAgentRadius = !bShowEntityAgentRadius; }
	void OnToggleShowEntityMoveTargets() { bShowEntityMoveTargets = !bShowEntityMoveTargets; }
	void OnToggleShowEntityLODDetails() { bShowEntityLODDetails = !bShowEntityLODDetails; }
	void OnToggleDebugLocalEntityManager();

	bool bShowEntityLocations = false;
	bool bShowEntityAgentRadius = false;
	bool bShowEntityMoveTargets = false;
	bool bShowEntityLODDetails = false;
	bool bDebugLocalEntityManager;
	int32 ToggleDebugLocalEntityManagerInputIndex = INDEX_NONE;

	struct FEntityDescription
	{
		FEntityDescription() = default;
		FEntityDescription(const float InScore, const FVector& InLocation, const FString& InDescription) : Score(InScore), Location(InLocation), Description(InDescription) {}

		float Score = 0.0f;
		FVector Location = FVector::ZeroVector;
		FString Description;
	};
	TArray<FEntityDescription> NearEntityDescriptions;

	FBinaryHeap<float> EntityViewDotHeap;
	TArray<float> EntityViewDots;
};

#endif // WITH_GAMEPLAY_DEBUGGER