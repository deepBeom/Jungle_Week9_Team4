#include "RenderCollector.h"

#include "Render/LineBatcher.h"
#include "Render/Renderer/RenderFlow/ShadowAtlasManager.h"
#include "GameFramework/World.h"
#include "GameFramework/AActor.h"
#include "Object/ActorIterator.h"
#include "Component/BillboardComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/GizmoComponent.h"
#include "Component/TextRenderComponent.h"
#include "Component/SubUVComponent.h"
#include "Component/DecalComponent.h"
#include "Component/HeightFogComponent.h"
#include "Component/SkyAtmosphereComponent.h"
#include "Component/Light/AmbientLightComponent.h"
#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Light/PointLightComponent.h"
#include "Component/Light/SpotLightComponent.h"
#include "Core/ResourceManager.h"
#include "Engine/Geometry/Frustum.h"
#include "Engine/Asset/StaticMesh.h"
#include "Engine/GameFramework/PrimitiveActors.h"
#include "Render/Resource/Material.h"
#include "Math/Utils.h"
#include "Object/ObjectIterator.h"
#include "Runtime/Stats/ScopeCycleCounter.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

namespace
{
	// ─────────────────── Billboard, SubUV ───────────────────
	FMatrix MakeViewBillboardMatrix(const UPrimitiveComponent* Primitive, const FRenderBus& RenderBus);
	FMatrix MakeViewSubUVSelectionMatrix(const USubUVComponent* SubUVComp, const FRenderBus& RenderBus);
	
	// ─────────────────── AABB, BVH ───────────────────
	bool UsesCameraDependentRenderBounds(const UPrimitiveComponent* PrimitiveComponent);
	FAABB BuildQuadAABB(const FMatrix& WorldMatrix);
	FAABB BuildRenderAABB(const UPrimitiveComponent* PrimitiveComponent, const FRenderBus& RenderBus);
	
	// ─────────────────── LOD ───────────────────
	int32 SelectLODLevel(const FVector& CameraPos, const FAABB& Bounds, const FMatrix& ProjMatrix, int32 ValidLODCount);
}

void FRenderCollector::ResetCullingStats()
{
	LastStats.Culling = {};
}

void FRenderCollector::ResetDecalStats()
{
	LastStats.Decal = {};
}

void FRenderCollector::ResetShadowStats()
{
	LastStats.Shadow = {};
}

// Frustum Culling을 통해 Light Collect와 Shadow Collect를 동시에 수행해줍니다.
void FRenderCollector::CollectLight(UWorld* World, FRenderBus& RenderBus, const FFrustum* ViewFrustum)
{
	LightRenderCollector.Collect(World, RenderBus, LastStats, ViewFrustum);
}

// 조명별 shadow 영향 볼륨으로 BVH를 조회해 shadow caster command를 수집합니다.
void FRenderCollector::CollectShadowCasters(UWorld* World, FRenderBus& RenderBus)
{
	if (World == nullptr)
	{
		return;
	}

	const EWorldType WorldType = World->GetWorldType();
	std::unordered_set<UPrimitiveComponent*> AddedPrimitives;

	auto AddShadowCaster = [&](UPrimitiveComponent* Primitive)
	{
		if (Primitive == nullptr || !Primitive->IsVisible()) return;
		if (Primitive->IsEditorOnly() && WorldType != EWorldType::Editor) return;
		if (Primitive->GetPrimitiveType() != EPrimitiveType::EPT_StaticMesh) return;
		if (!AddedPrimitives.insert(Primitive).second) return;

		UStaticMeshComponent* StaticMeshComp = static_cast<UStaticMeshComponent*>(Primitive);
		const UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();
		if (StaticMesh == nullptr || !StaticMesh->HasValidMeshData()) return;

		FMeshBuffer* MeshBuffer = MeshBufferManager.GetStaticMeshBuffer(StaticMesh, 0);
		if (MeshBuffer == nullptr || !MeshBuffer->IsValid()) return;

		const FStaticMesh* MeshData = StaticMesh->GetMeshData(0);
		if (MeshData == nullptr) return;

		for (const FStaticMeshSection& Section : MeshData->Sections)
		{
			FRenderCommand Cmd = {};
			Cmd.PerObjectConstants = FPerObjectConstants{ Primitive->GetWorldMatrix(), FColor::White().ToVector4() };
			Cmd.Type = ERenderCommandType::StaticMesh;
			Cmd.MeshBuffer = MeshBuffer;
			Cmd.SectionIndexStart = Section.StartIndex;
			Cmd.SectionIndexCount = Section.IndexCount;

			RenderBus.AddCommand(ERenderPass::ShadowCasters, Cmd);
		}
	};

	auto AddQueryResults = [&]()
	{
		for (UPrimitiveComponent* Primitive : VisiblePrimitiveScratch)
		{
			AddShadowCaster(Primitive);
		}
	};

	if (const FDirectionalShadowConstants* DirectionalShadow = RenderBus.GetDirectionalShadow())
	{
		const int32 DirectionalQueryCount =
			(DirectionalShadow->ShadowMode == DirectionalShadowModeValue::PSM) ? 1 : MAX_CASCADE_COUNT;
		for (int32 CascadeIndex = 0; CascadeIndex < DirectionalQueryCount; ++CascadeIndex)
		{
			FFrustum CascadeFrustum;
			CascadeFrustum.UpdateFromCamera(DirectionalShadow->LightViewProj[CascadeIndex]);
			World->GetSpatialIndex().FrustumQueryPrimitives(CascadeFrustum, VisiblePrimitiveScratch, FrustumQueryScratch);
			AddQueryResults();
		}
	}

	for (const FLightSlot& Slot : World->GetWorldLightSlots())
	{
		const ULightComponent* Light = Cast<ULightComponent>(Slot.LightData);
		if (!Slot.bAlive || Light == nullptr || !Light->IsVisible() || !Light->IsCastShadows()) continue;

		FVector Center = FVector::ZeroVector;
		float Radius = 0.0f;

		if (Light->GetLightType() == ELightType::LightType_Point)
		{
			const UPointLightComponent* PointLight = Cast<UPointLightComponent>(Light);
			if (PointLight == nullptr) continue;

			Center = PointLight->GetWorldLocation();
			Radius = PointLight->GetAttenuationRadius();
		}
		else if (Light->GetLightType() == ELightType::LightType_Spot)
		{
			const USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Light);
			if (SpotLight == nullptr) continue;

			const float SpotAngle = MathUtil::Clamp(std::max(SpotLight->GetOuterConeAngle(), SpotLight->GetInnerConeAngle()), 0.0f, 89.0f);
			Center = SpotLight->GetWorldLocation();
			Radius = SpotLight->GetAttenuationRadius();

			if (SpotAngle <= 45.0f)
			{
				const float Offset = Radius * 0.5f;
				const float BaseRadius = Radius * std::tan(MathUtil::DegreesToRadians(SpotAngle));
				Center += (SpotLight->GetUpVector() * -1.0f).GetSafeNormal() * Offset;
				Radius = std::sqrt((Offset * Offset) + (BaseRadius * BaseRadius));
			}
		}
		else
		{
			continue;
		}

		World->GetSpatialIndex().SphereQueryPrimitives(Center, Radius, VisiblePrimitiveScratch, SphereQueryScratch);
		AddQueryResults();
	}
}

void FRenderCollector::CollectSelection(const TArray<AActor*>& SelectedActors, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus)
{
	OverlayRenderCollector.CollectSelection(SelectedActors, ShowFlags, ViewMode, RenderBus, LineBatcher);
}

void FRenderCollector::CollectGrid(float GridSpacing, int32 GridHalfLineCount, FRenderBus& RenderBus, bool bOrthographic)
{
	OverlayRenderCollector.CollectGrid(GridSpacing, GridHalfLineCount, RenderBus, bOrthographic);
}

void FRenderCollector::CollectGizmo(UGizmoComponent* Gizmo, const FShowFlags& ShowFlags, FRenderBus& RenderBus, bool bIsActiveOperation)
{
	OverlayRenderCollector.CollectGizmo(Gizmo, ShowFlags, RenderBus, bIsActiveOperation);
}

void FRenderCollector::CollectFromActor(AActor* Actor, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType)
{
	if (!Actor->IsVisible()) return;

	for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
	{
		CollectFromComponent(Primitive, ShowFlags, ViewMode, RenderBus, WorldType);
	}
}

void FRenderCollector::CollectFromComponent(UPrimitiveComponent* Primitive, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus, EWorldType WorldType)
{
	if (!Primitive->IsVisible()) return;
	if (Primitive->IsEditorOnly() && WorldType != EWorldType::Editor) return;

	EPrimitiveType PrimType = Primitive->GetPrimitiveType();

	ID3D11ShaderResourceView* DefaultSRV = FResourceManager::Get().GetDefaultWhiteSRV();
	auto ResolveSRV = [&](const FString& Path) -> ID3D11ShaderResourceView*
		{
			UTexture* Texture = FResourceManager::Get().GetTexture(Path);
			return Texture ? Texture->GetSRV() : DefaultSRV;
		};
	static const FMaterial EngineDefaultMaterial{};

	switch (PrimType)
	{
	case EPrimitiveType::EPT_StaticMesh:
	{
		if (!ShowFlags.bPrimitives) return;

		UStaticMeshComponent* StaticMeshComp = static_cast<UStaticMeshComponent*>(Primitive);
		const UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();

		if (!StaticMesh || !StaticMesh->HasValidMeshData()) return;

		// 1. 카메라 정보 및 AABB 가져오기
		FVector CameraPos = RenderBus.GetCameraPosition();
		FMatrix ProjMatrix = RenderBus.GetProj();
		FAABB Bounds = StaticMeshComp->GetWorldAABB();
		const int32 ValidLODCount = StaticMesh->GetValidLODCount();

		// 2. LOD 레벨 계산
		int32 SelectedLOD = 0; // 기본값은 항상 원본(최고 화질)
		if (ShowFlags.bEnableLOD)
		{
			SelectedLOD = SelectLODLevel(CameraPos, Bounds, ProjMatrix, ValidLODCount);
		}

		FMeshBuffer* MeshBuffer = MeshBufferManager.GetStaticMeshBuffer(StaticMesh, SelectedLOD);
		if (!MeshBuffer) return;

		const FStaticMesh* MeshData = StaticMesh->GetMeshData(SelectedLOD);
		const TArray<FStaticMeshSection>& Sections = MeshData->Sections;

		for (int32 SectionIdx = 0; SectionIdx < static_cast<int32>(Sections.size()); ++SectionIdx)
		{
			const FStaticMeshSection& Section = Sections[SectionIdx];
			UMaterialInterface* Material = Cast<UMaterialInterface>(StaticMeshComp->GetMaterial(SectionIdx));
			if (Material == nullptr)
			{
				Material = FResourceManager::Get().GetMaterial("DefaultWhite");
				if (Material == nullptr)
				{
					continue;
				}
			}

			FRenderCommand Cmd = {};
			Cmd.PerObjectConstants = FPerObjectConstants{ Primitive->GetWorldMatrix(), FColor::White().ToVector4() };
			Cmd.Type = ERenderCommandType::StaticMesh;
			Cmd.MeshBuffer = MeshBuffer;

			Cmd.SectionIndexStart = Section.StartIndex;
			Cmd.SectionIndexCount = Section.IndexCount;
			Cmd.Material = Material;

			RenderBus.AddCommand(ERenderPass::Opaque, Cmd);

			if (Material->GetEffectiveLightingModel() == ELightingModel::Toon)
			{
				FRenderCommand OutlineCmd = {};
				OutlineCmd.Type = ERenderCommandType::ToonOutline;
				OutlineCmd.MeshBuffer = MeshBuffer;
				OutlineCmd.PerObjectConstants = FPerObjectConstants{
					Primitive->GetWorldMatrix()
				};
				OutlineCmd.SectionIndexStart = Section.StartIndex;
				OutlineCmd.SectionIndexCount = Section.IndexCount;
				OutlineCmd.Material = Material;

				RenderBus.AddCommand(ERenderPass::ToonOutline, OutlineCmd);
			}
		}

		break;
	}

	case EPrimitiveType::EPT_Text:
	{
		if (!ShowFlags.bBillboardText) return;

		UTextRenderComponent* TextComp = static_cast<UTextRenderComponent*>(Primitive);
		const FFontResource* Font = TextComp->GetFont();
		if (!Font || !Font->IsLoaded()) return;

		const FString& Text = TextComp->GetText();
		if (Text.empty()) return;

		FRenderCommand Cmd = {};
		Cmd.Type = ERenderCommandType::Font;
		Cmd.PerObjectConstants = FPerObjectConstants{TextComp->GetWorldMatrix(), TextComp->GetColor()};
		Cmd.Constants.Font.Text = &Text;
		Cmd.Constants.Font.Font = Font;
		Cmd.Constants.Font.Scale = TextComp->GetFontSize();
		
		RenderBus.AddCommand(ERenderPass::Font, Cmd);
		break;
	}

	case EPrimitiveType::EPT_SubUV:
	{
		USubUVComponent* SubUVComp = static_cast<USubUVComponent*>(Primitive);
		const FParticleResource* Particle = SubUVComp->GetParticle();
		if (!Particle || !Particle->IsLoaded()) return;

		FRenderCommand Cmd = {};
		Cmd.PerObjectConstants = FPerObjectConstants{
			MakeViewBillboardMatrix(Primitive, RenderBus),
			FColor::White().ToVector4() };
		Cmd.Type = ERenderCommandType::SubUV;
		Cmd.Constants.SubUV.Particle = Particle;
		Cmd.Constants.SubUV.FrameIndex = SubUVComp->GetFrameIndex();
		Cmd.Constants.SubUV.Width = SubUVComp->GetWidth();
		Cmd.Constants.SubUV.Height = SubUVComp->GetHeight();

		RenderBus.AddCommand(ERenderPass::SubUV, Cmd);
		break;
	}

	case EPrimitiveType::EPT_Billboard:
	{
		UBillboardComponent* BillboardComp = static_cast<UBillboardComponent*>(Primitive);
		UTexture* Texture = BillboardComp->GetTexture();

		UMaterial* BillboardMat = FResourceManager::Get().GetMaterial("BillboardMat");
		BillboardMat->DepthStencilType = EDepthStencilType::Default;
		BillboardMat->BlendType        = EBlendType::AlphaBlend;
		BillboardMat->RasterizerType   = ERasterizerType::SolidNoCull;
		BillboardMat->SamplerType      = ESamplerType::EST_Linear;

		const FMatrix BillboardMatrix = UBillboardComponent::MakeBillboardWorldMatrix(
			BillboardComp->GetWorldLocation(),
			FVector(0.01f, BillboardComp->GetWidth(), BillboardComp->GetHeight()),
			RenderBus.GetCameraForward(),
			RenderBus.GetCameraRight(),
			RenderBus.GetCameraUp());

		// LightComponents에 부착된 빌보드의 색상 조정
		FVector4 LightColor = FColor::White().ToVector4();
		if (ULightComponent* LightComponent = Cast<ULightComponent>(BillboardComp->GetParent()))
		{
			LightColor = LightComponent->GetLightColor().ToVector4();
		}

		FRenderCommand Cmd = {};
		Cmd.Type = ERenderCommandType::Billboard;
		Cmd.MeshBuffer = &MeshBufferManager.GetMeshBuffer(EPrimitiveType::EPT_Billboard);
		Cmd.PerObjectConstants = FPerObjectConstants{ BillboardMatrix, LightColor };
		Cmd.Material = BillboardMat;
		Cmd.Constants.Billboard.Texture = Texture;

		RenderBus.AddCommand(ERenderPass::Billboard, Cmd);
		break;
	}

	case EPrimitiveType::EPT_Decal:
	{
		if (!ShowFlags.bDecals) return;

		FScopeCycleCounter RenderDecalScope({});

		UDecalComponent* DecalComp = static_cast<UDecalComponent*>(Primitive);
		UMaterialInterface* Material = Cast<UMaterialInterface>(DecalComp->GetMaterial());

		UWorld* World = DecalComp->GetOwner() ? DecalComp->GetOwner()->GetFocusedWorld() : nullptr;

		FOBB DecalOBB = FOBB::FromAABB(DecalComp->GetWorldAABB(), DecalComp->GetWorldMatrix());

		TArray<UPrimitiveComponent*> VisiblePrimitiveScratch;
		World->GetSpatialIndex().OBBQueryPrimitives(DecalOBB, VisiblePrimitiveScratch, OBBQueryScratch);

		for (UPrimitiveComponent* Prim : VisiblePrimitiveScratch)
		{
			if (Prim->GetPrimitiveType() != EPrimitiveType::EPT_StaticMesh) continue;

			UStaticMeshComponent* StaticMeshComp = static_cast<UStaticMeshComponent*>(Prim);
			const UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh();

			if (!StaticMesh || !StaticMesh->HasValidMeshData()) continue;

			// 1. 카메라 정보 및 AABB 가져오기
			FVector CameraPos = RenderBus.GetCameraPosition();
			FMatrix ProjMatrix = RenderBus.GetProj();
			FAABB Bounds = StaticMeshComp->GetWorldAABB();
			const int32 ValidLODCount = StaticMesh->GetValidLODCount();

			int32 SelectedLOD = 0; // 기본값은 항상 원본(최고 화질)
			if (ShowFlags.bEnableLOD)
			{
				SelectedLOD = SelectLODLevel(CameraPos, Bounds, ProjMatrix, ValidLODCount);
			}

			FMeshBuffer* MeshBuffer = MeshBufferManager.GetStaticMeshBuffer(StaticMesh, SelectedLOD);
			if (!MeshBuffer) return;

			const FStaticMesh* MeshData = StaticMesh->GetMeshData(SelectedLOD);
			const TArray<FStaticMeshSection>& Sections = MeshData->Sections;

			for (int32 SectionIdx = 0; SectionIdx < static_cast<int32>(Sections.size()); ++SectionIdx)
			{
				const FStaticMeshSection& Section = Sections[SectionIdx];

				FRenderCommand Cmd = {};
				Cmd.Type = ERenderCommandType::Decal;
				Cmd.PerObjectConstants = FPerObjectConstants{ Prim->GetWorldMatrix(), DecalComp->GetDecalColor().ToVector4() };
				Cmd.MeshBuffer = MeshBuffer;

				Cmd.SectionIndexStart = Section.StartIndex;
				Cmd.SectionIndexCount = Section.IndexCount;

				Cmd.Material = Material;
				Cmd.Constants.Decal.InvDecalWorld = DecalComp->GetDecalMatrix().GetInverse();

				RenderBus.AddCommand(ERenderPass::Decal, Cmd);
			}
		}

		if (WorldType == EWorldType::Editor)
		{
			LineBatcher->AddOBB(DecalOBB, FColor::Green());
		}

		LastStats.Decal.TotalDecalCount += 1;
		LastStats.Decal.CollectTimeMS += static_cast<int32>(RenderDecalScope.Finish());
		break;
	}

	case EPrimitiveType::EPT_FOG:
	{
		if (!ShowFlags.bFog)
			return;
		UHeightFogComponent* HeightFogComp = static_cast<UHeightFogComponent*>(Primitive);

		FRenderCommand Cmd = {};
		Cmd.Type = ERenderCommandType::Primitive;
		Cmd.Constants.Fog.FogDensity = HeightFogComp->GetFogDensity();
		Cmd.Constants.Fog.FogColor = HeightFogComp->GetFogInscatteringColor();
		Cmd.Constants.Fog.HeightFalloff = HeightFogComp->GetHeightFalloff();
		Cmd.Constants.Fog.FogHeight = HeightFogComp->GetFogHeight();
		Cmd.Constants.Fog.FogStartDistance = HeightFogComp->GetFogStartDistance();
		Cmd.Constants.Fog.FogMaxOpacity = HeightFogComp->GetFogMaxOpacity();
		Cmd.Constants.Fog.FogCutoffDistance = HeightFogComp->GetFogCutoffDistance();
		//Cmd.BlendState = EBlendState::AlphaBlend;
		//Cmd.DepthStencilState = EDepthStencilState::Default;

		RenderBus.AddCommand(ERenderPass::Fog, Cmd);
		break;
	}
	case EPrimitiveType::EPT_SKY:
	{
		if (!RenderBus.GetCommands(ERenderPass::Sky).empty())
		{
			return;
		}

		USkyAtmosphereComponent* SkyComponent = static_cast<USkyAtmosphereComponent*>(Primitive);
		SkyComponent->RefreshSkyStateFromWorld();

		FRenderCommand Cmd = {};
		Cmd.Type = ERenderCommandType::Sky;
		SkyComponent->FillSkyConstants(RenderBus, Cmd.Constants.Sky);
		RenderBus.AddCommand(ERenderPass::Sky, Cmd);
		break;
	}
	default:
		if (PrimType == EPrimitiveType::EPT_TransGizmo || PrimType == EPrimitiveType::EPT_RotGizmo || PrimType == EPrimitiveType::EPT_ScaleGizmo)
		{
			return;
		}
		return;
	}
}

void FRenderCollector::CollectBVHInternalNodeAABBs(UPrimitiveComponent* PrimitiveComponent, const FShowFlags& ShowFlags, FRenderBus& RenderBus, std::unordered_set<int32>& SeenNodeIndices)
{
	OverlayRenderCollector.CollectBVHInternalNodeAABBs(PrimitiveComponent, ShowFlags, RenderBus, LineBatcher, SeenNodeIndices);
}

void FRenderCollector::CollectWorld(UWorld* World, const FShowFlags& ShowFlags, EViewMode ViewMode, FRenderBus& RenderBus,
	const FFrustum* ViewFrustum)
{
	ResetCullingStats();
	ResetDecalStats();
	ResetShadowStats();

	if (!World) return;
	
	CollectLight(World, RenderBus, ViewFrustum);
	CollectShadowCasters(World, RenderBus);

	if (ViewFrustum)
	{
		VisiblePrimitiveScratch.clear();
		World->GetSpatialIndex().FrustumQueryPrimitives(*ViewFrustum, VisiblePrimitiveScratch, FrustumQueryScratch);

		for (UPrimitiveComponent* Primitive : VisiblePrimitiveScratch)
		{
			if (Primitive == nullptr || UsesCameraDependentRenderBounds(Primitive) || !Primitive->IsEnableCull()) continue;
			++LastStats.Culling.BVHPassedPrimitiveCount;
			CollectFromComponent(Primitive, ShowFlags, ViewMode, RenderBus, World->GetWorldType());
		}
	}

	std::unordered_set<UPrimitiveComponent*> CollectCameraDependentPrimitives;
	if (ViewFrustum)
	{
		CollectCameraDependentPrimitives.reserve(32);
	}
	
	// Frustum이 없다면 액터 단위로 통째로 수집하고, 그렇지 않다면 BVH에서 누락된 컴포넌트들을 개별 수집
	for (TActorIterator<AActor> Iter(World); Iter; ++Iter)
	{
		AActor* Actor = *Iter;
		if (!Actor || !Actor->IsVisible()) continue;

		if (!ViewFrustum)
		{
			for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
			{	
				if (Primitive != nullptr && !Primitive->IsVisible())
				{
					++LastStats.Culling.TotalVisiblePrimitiveCount;
				}
			}
			CollectFromActor(Actor, ShowFlags, ViewMode, RenderBus, World->GetWorldType());
			continue; // early-continue
		}

		// 이미 처리된 컴포넌트, 중복된 컴포넌트는 제외하고 Frustum Culling 수행
		for (UPrimitiveComponent* Primitive : Actor->GetPrimitiveComponents())
		{
			if (!Primitive || !Primitive->IsVisible()) continue;

			++LastStats.Culling.TotalVisiblePrimitiveCount;

			const bool bIsCameraDependent = UsesCameraDependentRenderBounds(Primitive);
			if (!bIsCameraDependent && Primitive->IsEnableCull()) continue;
			if (!CollectCameraDependentPrimitives.insert(Primitive).second) continue;

			if (bIsCameraDependent && Primitive->IsEnableCull())
			{
				if (ViewFrustum->Intersects(BuildRenderAABB(Primitive, RenderBus)) == FFrustum::EFrustumIntersectResult::Outside)
					continue;
			}

			++LastStats.Culling.FallbackPassedPrimitiveCount;
			CollectFromComponent(Primitive, ShowFlags, ViewMode, RenderBus, World->GetWorldType());
		}
	}
}

// ─────────────────── namespace ────────────────────────────────────────────────────────────

namespace
{
	bool UsesCameraDependentRenderBounds(const UPrimitiveComponent* PrimitiveComponent)
	{
		if (PrimitiveComponent == nullptr)
		{
			return false;
		}

		switch (PrimitiveComponent->GetPrimitiveType())
		{
		case EPrimitiveType::EPT_Billboard:
		case EPrimitiveType::EPT_Text:
		case EPrimitiveType::EPT_SubUV:
			return true;
		default:
			return false;
		}
	}

	FMatrix MakeViewBillboardMatrix(const UPrimitiveComponent* Primitive, const FRenderBus& RenderBus)
	{
		const FMatrix WorldMatrix = Primitive->GetWorldMatrix();
		return UBillboardComponent::MakeBillboardWorldMatrix(
			WorldMatrix.GetOrigin(),
			WorldMatrix.GetScaleVector(),
			RenderBus.GetCameraForward(),
			RenderBus.GetCameraRight(),
			RenderBus.GetCameraUp());
	}

	FMatrix MakeViewSubUVSelectionMatrix(const USubUVComponent* SubUVComp, const FRenderBus& RenderBus)
	{
		const FVector WorldScale = SubUVComp->GetWorldScale();
		return USubUVComponent::MakeBillboardWorldMatrix(
			SubUVComp->GetWorldLocation(),
			FVector(
				WorldScale.X > 0.01f ? WorldScale.X : 0.01f,
				SubUVComp->GetWidth() * WorldScale.Y,
				SubUVComp->GetHeight() * WorldScale.Z),
			RenderBus.GetCameraForward(),
			RenderBus.GetCameraRight(),
			RenderBus.GetCameraUp());
	}
	/*
	* BillBoardComponent를 상속받은 text, SubUV가 사용하는 AABB 계산함수(의존성 분리)
	*/
	FAABB BuildQuadAABB(const FMatrix& WorldMatrix)
	{
		static constexpr FVector LocalQuadCorners[4] =
		{
			FVector(0.0f, -0.5f,  0.5f),
			FVector(0.0f,  0.5f,  0.5f),
			FVector(0.0f,  0.5f, -0.5f),
			FVector(0.0f, -0.5f, -0.5f)
		};

		FAABB Box;
		Box.Reset();

		for (const FVector& Corner : LocalQuadCorners)
		{
			Box.Expand(WorldMatrix.TransformPosition(Corner));
		}

		return Box;
	}

	FAABB BuildRenderAABB(const UPrimitiveComponent* PrimitiveComponent, const FRenderBus& RenderBus)
	{
		switch (PrimitiveComponent->GetPrimitiveType())
		{
		case EPrimitiveType::EPT_Billboard:
		{
			const UBillboardComponent* BillboardComponent = static_cast<const UBillboardComponent*>(PrimitiveComponent);
			return BuildQuadAABB(UBillboardComponent::MakeBillboardWorldMatrix(
				BillboardComponent->GetWorldLocation(),
				FVector(0.00f, BillboardComponent->GetWidth(), BillboardComponent->GetHeight()),
				RenderBus.GetCameraForward(),
				RenderBus.GetCameraRight(),
				RenderBus.GetCameraUp()));
		}
		case EPrimitiveType::EPT_Text:
		{
			const UTextRenderComponent* TextComp = static_cast<const UTextRenderComponent*>(PrimitiveComponent);
			return BuildQuadAABB(TextComp->GetTextMatrix());
		}
		case EPrimitiveType::EPT_SubUV:
		{
			const USubUVComponent* SubUVComp = static_cast<const USubUVComponent*>(PrimitiveComponent);
			return BuildQuadAABB(MakeViewSubUVSelectionMatrix(SubUVComp, RenderBus));
		}

		default:
			return PrimitiveComponent->GetWorldAABB();
		}
	}

	int32 SelectLODLevel(const FVector& CameraPos, const FAABB& Bounds, const FMatrix& ProjMatrix, int32 ValidLODCount)
	{
		bool IsOrthoGraphic = (std::abs(ProjMatrix.M[3][3] - 1.0f) < 1e-4f);
		if (ValidLODCount <= 1 || IsOrthoGraphic) return 0;

		// 1. 바운딩 박스를 통해 바운딩 스피어 반지름 및 카메라와의 거리 계산
		const FVector Center = (Bounds.Min + Bounds.Max) * 0.5f;
		const FVector Extent = (Bounds.Max - Bounds.Min) * 0.5f;
		const float SphereRadius = std::sqrt(Extent.X * Extent.X + Extent.Y * Extent.Y + Extent.Z * Extent.Z);

		const FVector Diff = Center - CameraPos;
		const float Dist = std::sqrt(Diff.X * Diff.X + Diff.Y * Diff.Y + Diff.Z * Diff.Z);

		if (Dist <= 1e-4f) return 0;

		const float ProjectedRadius = (SphereRadius / Dist) * ProjMatrix.M[2][1];
		const float ScreenCoverage = ProjectedRadius; 

		static constexpr float Thresholds[] = { 0.15f, 0.08f, 0.05f, 0.02f };
		static constexpr int32 ThresholdCount = static_cast<int32>(sizeof(Thresholds) / sizeof(Thresholds[0]));

		const int32 MaxLOD = ValidLODCount - 1;
		for (int32 LOD = 0; LOD < MaxLOD; ++LOD)
		{
			float Threshold = (LOD < ThresholdCount) ? Thresholds[LOD] : 0.0f;
			if (ScreenCoverage >= Threshold)
				return LOD;
		}

		// 화면에 차지하는 비율이 가장 낮을 경우 최하위 LOD 반환
		return MaxLOD;
	}
}
