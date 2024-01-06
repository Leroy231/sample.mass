// Copyright (c) Haywire Interactive, LLC. All Rights Reserved.
// Adapted from GameplayDebuggerCategory_Mass.cpp.

#include "GameplayDebuggerCategory_ProjectM.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include <MassCommonFragments.h>
#include <GameplayDebuggerConfig.h>
#include "CanvasItem.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassRepresentationFragments.h"
#include "MassLODSubsystem.h"
#include "MassRepresentationSubsystem.h"
#include "MassNavigationFragments.h"

namespace PM::GameplayDebuggerCategory_ProjectM::Tweakables
{
	bool bFilterGameplayDebuggerToDebugEntity = false;
	bool bShowLocationWithForceAndVelocity = false;

	FAutoConsoleVariableRef CVars[] =
	{
		FAutoConsoleVariableRef(TEXT("pm.FilterGameplayDebuggerToDebugEntity"), bFilterGameplayDebuggerToDebugEntity, TEXT("FilterGameplayDebuggerToDebugEntity"), ECVF_Cheat),
		FAutoConsoleVariableRef(TEXT("pm.ShowLocationWithForceAndVelocity"), bShowLocationWithForceAndVelocity, TEXT("ShowLocationWithForceAndVelocity"), ECVF_Cheat)
	};
}

FGameplayDebuggerCategory_ProjectM::FGameplayDebuggerCategory_ProjectM()
{
	bShowOnlyWithDebugActor = false;
	bDebugLocalEntityManager = false;

	BindKeyPress(EKeys::T.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FGameplayDebuggerCategory_ProjectM::OnToggleShowEntityLocations, EGameplayDebuggerInputMode::Replicated);
	BindKeyPress(EKeys::R.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FGameplayDebuggerCategory_ProjectM::OnToggleShowEntityAgentRadii, EGameplayDebuggerInputMode::Replicated);
	BindKeyPress(EKeys::M.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FGameplayDebuggerCategory_ProjectM::OnToggleShowEntityMoveTargets, EGameplayDebuggerInputMode::Replicated);
	BindKeyPress(EKeys::K.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FGameplayDebuggerCategory_ProjectM::OnToggleShowEntityLODDetails, EGameplayDebuggerInputMode::Replicated);
	ToggleDebugLocalEntityManagerInputIndex = GetNumInputHandlers();
	BindKeyPress(EKeys::L.GetFName(), FGameplayDebuggerInputModifier::Shift, this, &FGameplayDebuggerCategory_ProjectM::OnToggleDebugLocalEntityManager, EGameplayDebuggerInputMode::Local);
}

// Warning: This gets called on every tick before UMassProcessor::Execute.
void FGameplayDebuggerCategory_ProjectM::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (!OwnerPC)
	{
		return;
	}

	if (bAllowLocalDataCollection)
	{
		ResetReplicatedData();
	}

	// we only want to display this if there are local/remote roles in play
	if (IsCategoryAuth() != IsCategoryLocal())
	{
		AddTextLine(FString::Printf(TEXT("Source: {yellow}%s{white}"), bDebugLocalEntityManager ? TEXT("LOCAL") : TEXT("REMOTE")));
	}

	NearEntityDescriptions.Reset();
	EntityViewDotHeap.Clear();
	EntityViewDots.Reset();

	FVector ViewLocation = FVector::ZeroVector;
	FVector ViewDirection = FVector::ForwardVector;
	ensureMsgf(GetViewPoint(OwnerPC, ViewLocation, ViewDirection), TEXT("GetViewPoint is expected to always succeed when passing a valid controller."));

	CollectDataForEntities(OwnerPC, ViewLocation, ViewDirection);
}

void FGameplayDebuggerCategory_ProjectM::DrawTargetEntityLocations(const TArray<FVector>& TargetEntityLocations, const FColor& Color, const FVector& EntityLocation, const bool bIncludeDistance)
{
	const FVector ZOffset(0.f, 0.f, 200.f);
	for (const FVector& TargetEntityLocation : TargetEntityLocations)
	{
		FString DistanceString = bIncludeDistance ? FString::Printf(TEXT("Distance: %.1f"), FVector::Dist(TargetEntityLocation, EntityLocation)) : FString();
		AddShape(FGameplayDebuggerShape::MakeArrow(EntityLocation + ZOffset, TargetEntityLocation + ZOffset, 10.0f, 2.0f, Color, DistanceString));
		AddShape(FGameplayDebuggerShape::MakeCylinder(TargetEntityLocation + ZOffset / 2.f, 50.f, 100.0f, Color));
	}
}

void FGameplayDebuggerCategory_ProjectM::OnToggleDebugLocalEntityManager()
{
	// this code will only execute on locally-controlled categories (as per BindKeyPress's EGameplayDebuggerInputMode::Local
	// parameter). In such a case we don't want to toggle if we're also Auth (there's no client-server relationship here).
	if (IsCategoryAuth())
	{
		return;
	}

	ResetReplicatedData();
	bDebugLocalEntityManager = !bDebugLocalEntityManager;
	bAllowLocalDataCollection = bDebugLocalEntityManager;

	const EGameplayDebuggerInputMode NewInputMode = bDebugLocalEntityManager ? EGameplayDebuggerInputMode::Local : EGameplayDebuggerInputMode::Replicated;
	for (int32 HandlerIndex = 0; HandlerIndex < GetNumInputHandlers(); ++HandlerIndex)
	{
		if (HandlerIndex != ToggleDebugLocalEntityManagerInputIndex)
		{
			GetInputHandler(HandlerIndex).Mode = NewInputMode;
		}
	}
}

void FGameplayDebuggerCategory_ProjectM::CollectDataForEntities(const APlayerController* OwnerPC, const FVector& ViewLocation, const FVector& ViewDirection)
{
	const UWorld* World = GetDataWorld(OwnerPC, nullptr);

	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(World);
	if (!EntityManager)
	{
		return;
	}

	FMassEntityQuery EntityQuery;
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassForceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);

	FMassExecutionContext Context(*EntityManager, 0.0f);

	const UMassLODSubsystem* LODSubsystem = World->GetSubsystem<UMassLODSubsystem>();
	const FVector FirstViewerLocation = LODSubsystem->GetViewers()[0].Location;

	EntityQuery.ForEachEntityChunk(*EntityManager, Context, [this, &ViewLocation, &ViewDirection, &FirstViewerLocation](FMassExecutionContext& Context)
	{
		const int32 NumEntities = Context.GetNumEntities();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Context.GetFragmentView<FMassMoveTargetFragment>();
		const TConstArrayView<FAgentRadiusFragment> RadiusList = Context.GetFragmentView<FAgentRadiusFragment>();
		const TConstArrayView<FMassForceFragment> ForceList = Context.GetFragmentView<FMassForceFragment>();
		const TConstArrayView<FMassVelocityFragment> VelocityList = Context.GetFragmentView<FMassVelocityFragment>();
		const TConstArrayView<FMassRepresentationFragment> RepresentationList = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();

		const UGameplayDebuggerUserSettings* Settings = GetDefault<UGameplayDebuggerUserSettings>();
		const float MaxViewDistance = Settings->MaxViewDistance;
		const float MinViewDirDot = FMath::Cos(FMath::DegreesToRadians(Settings->MaxViewAngle));

		for (int32 EntityIndex = 0; EntityIndex < NumEntities; ++EntityIndex)
		{
			CollectDataForEntity(TransformList[EntityIndex].GetTransform(), MinViewDirDot, ViewLocation, ViewDirection, MaxViewDistance, MoveTargetList.IsEmpty() ? nullptr : &MoveTargetList[EntityIndex], RadiusList[EntityIndex].Radius, Context.GetEntity(EntityIndex), ForceList[EntityIndex], VelocityList[EntityIndex], RepresentationList[EntityIndex], RepresentationLODList[EntityIndex], FirstViewerLocation);
		}
	});
}

FString MassVisibilityToString(const EMassVisibility Visibility)
{
	switch (Visibility)
	{
	case EMassVisibility::CanBeSeen: return TEXT("CanBeSeen");
	case EMassVisibility::CulledByFrustum: return TEXT("CulledByFrustum");
	case EMassVisibility::CulledByDistance: return TEXT("CulledByDistance");
	case EMassVisibility::Max: return TEXT("Max");
	default: return TEXT("");
	}
}

void FGameplayDebuggerCategory_ProjectM::CollectDataForEntity(const FTransform& Transform, const float MinViewDirDot, const FVector& ViewLocation, const FVector& ViewDirection, const float MaxViewDistance, const FMassMoveTargetFragment* MoveTargetFragment, const float AgentRadius, const FMassEntityHandle& Entity, const FMassForceFragment& ForceFragment, const FMassVelocityFragment& VelocityFragment, const FMassRepresentationFragment& RepresentationFragment, const FMassRepresentationLODFragment& RepresentationLODFragment, const FVector& FirstViewerLocation)
{
	if (PM::GameplayDebuggerCategory_ProjectM::Tweakables::bFilterGameplayDebuggerToDebugEntity && !UE::Mass::Debug::IsDebuggingEntity(Entity))
	{
		return;
	}

	// Cull entity if needed
	const FVector& EntityLocation = Transform.GetLocation();
	const FVector DirToEntity = EntityLocation - ViewLocation;
	const float DistanceToEntitySq = DirToEntity.SquaredLength();
	if (DistanceToEntitySq > FMath::Square(MaxViewDistance))
	{
		return;
	}

	const float ViewDot = FVector::DotProduct(DirToEntity.GetSafeNormal(), ViewDirection);
	if (ViewDot < MinViewDirDot)
	{
		return;
	}

	static constexpr int32 MaxLabels = 15;
	if (EntityViewDots.Num() < MaxLabels)
	{
		EntityViewDots.Add(ViewDot);
		EntityViewDotHeap.Add(ViewDot, EntityViewDots.Num() - 1);
	}
	else
	{
		if (EntityViewDots[EntityViewDotHeap.Top()] > ViewDot)
		{
			return;
		}

		EntityViewDotHeap.Pop();
		EntityViewDots.Add(ViewDot);
		EntityViewDotHeap.Add(ViewDot, EntityViewDots.Num() - 1);
	}

	if (bShowEntityLocations)
	{
		if (PM::GameplayDebuggerCategory_ProjectM::Tweakables::bShowLocationWithForceAndVelocity)
		{
			const float Force = ForceFragment.Value.Size();
			const float Velocity = VelocityFragment.Value.Size();
			AddShape(FGameplayDebuggerShape::MakePoint(EntityLocation, 3.f, FColor::Magenta, FString::Printf(TEXT("{magenta}[%s]\n%s\nF: %.1f\nV: %.1f"), *Entity.DebugGetDescription(), *EntityLocation.ToCompactString(), Force, Velocity)));
		}
		else
		{
			AddShape(FGameplayDebuggerShape::MakePoint(EntityLocation, 3.f, FColor::Magenta, FString::Printf(TEXT("{magenta}[%s]"), *Entity.DebugGetDescription())));
		}
	}

	if(bShowEntityAgentRadius)
	{
		AddShape(FGameplayDebuggerShape::MakeCircle(EntityLocation, FVector(0, 0, 1), AgentRadius, FColor::Green, FString::Printf(TEXT("{magenta}[%s]\nRadius: %.1f"), *Entity.DebugGetDescription(), AgentRadius)));
	}

	if (bShowEntityLODDetails)
	{
		const float ClosestViewerDistance = FVector::Dist(EntityLocation, FirstViewerLocation);

		AddShape(FGameplayDebuggerShape::MakePoint(EntityLocation, 3.f, FColor::Magenta, FString::Printf(TEXT("{magenta}[%s]\nLOD: %s\nClosestViewerDistance: %.1f\nRepresentation: %s\nVisibility: %s"), *Entity.DebugGetDescription(), *UEnum::GetDisplayValueAsText(RepresentationLODFragment.LOD).ToString(), ClosestViewerDistance, *UEnum::GetDisplayValueAsText(RepresentationFragment.CurrentRepresentation).ToString(), *MassVisibilityToString(RepresentationLODFragment.Visibility))));
	}

	if (MoveTargetFragment && bShowEntityMoveTargets)
	{
		// Adjust vertically to avoid overlap with ground.
		const FVector MoveTargetCenterWithVerticalOffset = MoveTargetFragment->Center + FVector(0.f, 0.f, 10.f);
		AddShape(FGameplayDebuggerShape::MakePoint(MoveTargetCenterWithVerticalOffset, 3.f, FColor::Black, FString(TEXT("{black}Move Target Center"))));
		AddShape(FGameplayDebuggerShape::MakeArrow(EntityLocation, MoveTargetCenterWithVerticalOffset, 10.f, 2.f, FColor::Black));

		AddShape(FGameplayDebuggerShape::MakeArrow(MoveTargetCenterWithVerticalOffset, MoveTargetCenterWithVerticalOffset + MoveTargetFragment->Forward * AgentRadius, 10.f, 2.f, FColor::Green, FString(TEXT("Move Target Forward"))));
	}

	// Cap labels to closest ones.
	NearEntityDescriptions.Sort([](const FEntityDescription& LHS, const FEntityDescription& RHS) { return LHS.Score < RHS.Score; });
	if (NearEntityDescriptions.Num() > MaxLabels)
	{
		NearEntityDescriptions.RemoveAt(MaxLabels, NearEntityDescriptions.Num() - MaxLabels);
	}
}

void FGameplayDebuggerCategory_ProjectM::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.Printf(TEXT("\n[{yellow}%s{white}] %s Entity locations"), *GetInputHandlerDescription(0), bShowEntityLocations ? TEXT("Hide") : TEXT("Show"));
	CanvasContext.Printf(TEXT("\n[{yellow}%s{white}] %s Entity radii"), *GetInputHandlerDescription(1), bShowEntityAgentRadius ? TEXT("Hide") : TEXT("Show"));
	CanvasContext.Printf(TEXT("\n[{yellow}%s{white}] %s Entity move targets"), *GetInputHandlerDescription(2), bShowEntityMoveTargets ? TEXT("Hide") : TEXT("Show"));
	CanvasContext.Printf(TEXT("\n[{yellow}%s{white}] %s Entity LOD details"), *GetInputHandlerDescription(3), bShowEntityLODDetails ? TEXT("Hide") : TEXT("Show"));

	if (IsCategoryLocal() && !IsCategoryAuth())
	{
		// we want to display this line only on clients in client-server environment.
		CanvasContext.Printf(TEXT("[{yellow}%s{white}] Toggle Local/Remote debugging"), *GetInputHandlerDescription(ToggleDebugLocalEntityManagerInputIndex));
	}

	struct FEntityLayoutRect
	{
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
		int32 Index = INDEX_NONE;
		float Alpha = 1.0f;
	};

	TArray<FEntityLayoutRect> Layout;

	// The loop below is O(N^2), make sure to keep the N small.
	constexpr int32 MaxDesc = 20;
	const int32 NumDescs = FMath::Min(NearEntityDescriptions.Num(), MaxDesc);

	// The labels are assumed to have been ordered in order of importance (i.e. front to back).
	for (int32 Index = 0; Index < NumDescs; Index++)
	{
		const FEntityDescription& Desc = NearEntityDescriptions[Index];
		if (Desc.Description.Len() && CanvasContext.IsLocationVisible(Desc.Location))
		{
			float SizeX = 0, SizeY = 0;
			const FVector2D ScreenLocation = CanvasContext.ProjectLocation(Desc.Location);
			CanvasContext.MeasureString(Desc.Description, SizeX, SizeY);

			FEntityLayoutRect Rect;
			Rect.Min = ScreenLocation + FVector2D(0, -SizeY * 0.5f);
			Rect.Max = Rect.Min + FVector2D(SizeX, SizeY);
			Rect.Index = Index;
			Rect.Alpha = 0.0f;

			// Calculate transparency based on how much more important rects are overlapping the new rect.
			const float Area = FMath::Max(0.0f, Rect.Max.X - Rect.Min.X) * FMath::Max(0.0f, Rect.Max.Y - Rect.Min.Y);
			const float InvArea = Area > KINDA_SMALL_NUMBER ? 1.0f / Area : 0.0f;
			float Coverage = 0.0;

			for (const FEntityLayoutRect& Other : Layout)
			{
				// Calculate rect intersection
				const float MinX = FMath::Max(Rect.Min.X, Other.Min.X);
				const float MinY = FMath::Max(Rect.Min.Y, Other.Min.Y);
				const float MaxX = FMath::Min(Rect.Max.X, Other.Max.X);
				const float MaxY = FMath::Min(Rect.Max.Y, Other.Max.Y);

				// return zero area if not overlapping
				const float IntersectingArea = FMath::Max(0.0f, MaxX - MinX) * FMath::Max(0.0f, MaxY - MinY);
				Coverage += (IntersectingArea * InvArea) * Other.Alpha;
			}

			Rect.Alpha = FMath::Square(1.0f - FMath::Min(Coverage, 1.0f));

			if (Rect.Alpha > KINDA_SMALL_NUMBER)
			{
				Layout.Add(Rect);
			}
		}
	}

	// Render back to front so that the most important item renders at top.
	const FVector2D Padding(5, 5);
	for (int32 Index = Layout.Num() - 1; Index >= 0; Index--)
	{
		const FEntityLayoutRect& Rect = Layout[Index];
		const FEntityDescription& Desc = NearEntityDescriptions[Rect.Index];

		const FVector2D BackgroundPosition(Rect.Min - Padding);
		FCanvasTileItem Background(Rect.Min - Padding, Rect.Max - Rect.Min + Padding * 2.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.35f * Rect.Alpha));
		Background.BlendMode = SE_BLEND_TranslucentAlphaOnly;
		CanvasContext.DrawItem(Background, BackgroundPosition.X, BackgroundPosition.Y);

		CanvasContext.PrintAt(Rect.Min.X, Rect.Min.Y, FColor::White, Rect.Alpha, Desc.Description);
	}

	FGameplayDebuggerCategory::DrawData(OwnerPC, CanvasContext);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_ProjectM::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_ProjectM());
}

#endif // WITH_GAMEPLAY_DEBUGGER
