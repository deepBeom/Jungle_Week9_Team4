#pragma once
#include "Render/Collector/LightRenderCollector.h"
#include "Render/Collector/OverlayRenderCollector.h"
#include "Render/Collector/RenderCollectionStats.h"
#include "Render/Scene/RenderBus.h"
#include "Render/Resource/MeshBufferManager.h"
#include "Spatial/WorldSpatialIndex.h"
#include "Geometry/OBB.h"
#include <unordered_set>

enum class EWorldType : uint32;

class UWorld;
class AActor;
class ASpotLightActor;
class UPrimitiveComponent;
class UGizmoComponent;
class UDecalComponent;
class FLineBatcher;
struct FFrustum;

class FRenderCollector {
private:
	FMeshBufferManager MeshBufferManager;
	FLightRenderCollector LightRenderCollector;
	FOverlayRenderCollector OverlayRenderCollector;
	FLineBatcher* LineBatcher = nullptr;

	// ────── Collects ─────────────────────────────────────────────────────────
public:
	void Initialize(ID3D11Device* InDevice) { MeshBufferManager.Create(InDevice); OverlayRenderCollector.Initialize(&MeshBufferManager); }
	void Release() { LineBatcher = nullptr; OverlayRenderCollector.Release(); MeshBufferManager.Release(); }
	void SetLineBatcher(FLineBatcher* InLineBatcher) { LineBatcher = InLineBatcher; }
	void ClearLineBatcher() { LineBatcher = nullptr; }

	void CollectWorld(UWorld* World, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, const FFrustum* ViewFrustum = nullptr);
	void CollectSelection(const TArray<AActor*>& SelectedActors, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectGizmo(UGizmoComponent* Gizmo, const FShowFlags& ShowFlags, FRenderBus& RenderBus, bool bIsActiveOperation);
	void CollectGrid(float GridSpacing, int32 GridHalfLineCount, FRenderBus& RenderBus, bool bOrthographic = false);

private:
	void CollectLight(UWorld* World, FRenderBus& RenderBus, const FFrustum* ViewFrustum = nullptr);
	void CollectShadowCasters(UWorld* World, FRenderBus& RenderBus);
	void CollectFromActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType);
	void CollectFromComponent(UPrimitiveComponent* Primitive, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType);
	void CollectBVHInternalNodeAABBs(UPrimitiveComponent* PrimitiveComponent, const FShowFlags& ShowFlags, FRenderBus& RenderBus, std::unordered_set<int32>& SeenNodeIndices);

	// ────── Stats ────────────────────────────────────────────────────────────
private:
	FRenderCollectionStats LastStats;
	void ResetCullingStats();
	void ResetDecalStats();
	void ResetShadowStats();

public:
	const FRenderCollectionStats& GetLastStats() const { return LastStats; }
	const FCullingStats& GetLastCullingStats() const { return LastStats.Culling; }
	const FDecalStats& GetLastDecalStats() const { return LastStats.Decal; }
	const FShadowStats& GetLastShadowStats() const { return LastStats.Shadow; }

	using FCullingStats = ::FCullingStats;
	using FDecalStats = ::FDecalStats;
	using FShadowStats = ::FShadowStats;
	using FRenderCollectionStats = ::FRenderCollectionStats;

	// ────── Query Buffer ─────────────────────────────────────────────────────
private:
	// WorldSpatialIndex culling queries reuse these scratch buffers each frame.
	FWorldSpatialIndex::FPrimitiveFrustumQueryScratch FrustumQueryScratch;
	FWorldSpatialIndex::FPrimitiveOBBQueryScratch OBBQueryScratch;
	FWorldSpatialIndex::FPrimitiveSphereQueryScratch SphereQueryScratch;
	// Spatial query results are stored here temporarily before render commands are emitted.
	TArray<UPrimitiveComponent*> VisiblePrimitiveScratch;	
};
