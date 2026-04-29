#pragma once
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
class UFireballComponent;
class FLineBatcher;
struct FFrustum;

class FRenderCollector {
public:
	using FCullingStats = ::FCullingStats;
	using FDecalStats = ::FDecalStats;
	using FShadowStats = ::FShadowStats;
	using FRenderCollectionStats = ::FRenderCollectionStats;

private:
	FMeshBufferManager MeshBufferManager;
	FLineBatcher* LineBatcher = nullptr;
	// WorldSpatialIndex 컬링 쿼리용 scratch buffer
	FWorldSpatialIndex::FPrimitiveFrustumQueryScratch FrustumQueryScratch;
	FWorldSpatialIndex::FPrimitiveOBBQueryScratch OBBQueryScratch;
	FWorldSpatialIndex::FPrimitiveSphereQueryScratch SphereQueryScratch;
	// spatial query 결과를 받아 render command 생성 전 임시로 들고 있는 목록
	TArray<UPrimitiveComponent*> VisiblePrimitiveScratch;	
	FRenderCollectionStats LastStats;

public:
	void Initialize(ID3D11Device* InDevice) { MeshBufferManager.Create(InDevice); }
	void Release() { LineBatcher = nullptr; MeshBufferManager.Release(); }
	void SetLineBatcher(FLineBatcher* InLineBatcher) { LineBatcher = InLineBatcher; }
	void ClearLineBatcher() { LineBatcher = nullptr; }

	void CollectWorld(UWorld* World, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus,
	                  const FFrustum* ViewFrustum = nullptr);
	void CollectSelection(const TArray<AActor*>& SelectedActors, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectGizmo(UGizmoComponent* Gizmo, const FShowFlags& ShowFlags, FRenderBus& RenderBus, bool bIsActiveOperation);
	void CollectGrid(float GridSpacing, int32 GridHalfLineCount, FRenderBus& RenderBus, bool bOrthographic = false);
	const FRenderCollectionStats& GetLastStats() const { return LastStats; }
	const FCullingStats& GetLastCullingStats() const { return LastStats.Culling; }
	const FDecalStats& GetLastDecalStats() const { return LastStats.Decal; }
	const FShadowStats& GetLastShadowStats() const { return LastStats.Shadow; }

private:
	void ResetCullingStats();
	void ResetDecalStats();
	void ResetShadowStats();

	void CollectLight(UWorld* World, FRenderBus& RenderBus, const FFrustum* ViewFrustum = nullptr);
	// Collect shadow caster draw commands by querying the BVH with each light's shadow volume.
	void CollectShadowCasters(UWorld* World, FRenderBus& RenderBus);
	void CollectFromActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType);
	bool CollectFromSelectedActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus);
	void CollectFromComponent(UPrimitiveComponent* Primitive, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType);
	void CollectBVHInternalNodeAABBs(UPrimitiveComponent* PrimitiveComponent, const FShowFlags& ShowFlags, FRenderBus& RenderBus, std::unordered_set<int32>& SeenNodeIndices);
};
