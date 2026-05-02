#pragma once

#include "Core/CoreTypes.h"
#include "Core/Containers/Array.h"
#include "Core/ResourceTypes.h"
#include "Render/Common/ComPtr.h"
#include "Render/Resource/Material.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
class FRenderBus;

// NDC 좌표 + UV + 색상 — SubUVBatcher의 FTextureVertex와 달리 2D + 색상 포함
struct FUIVertex
{
    float X, Y;        // NDC 좌표 (-1 ~ +1), AddQuad 시점에 픽셀에서 변환
    float U, V;        // UV
    float R, G, B, A;  // 색상 tint (텍스처 없는 단색 쿼드도 이걸로 처리)
};

// 텍스처가 같은 쿼드를 묶어 DrawIndexed 한 번으로 처리
// SubUVBatcher의 FSRVBatch와 동일한 역할
struct FUIBatch
{
    UTexture* Texture;   // nullptr 이면 1x1 흰 텍스처로 단색 처리
    uint32    IndexStart;
    uint32    IndexCount;
    int32     BaseVertex;
};

// FUIBatcher — 스크린 스페이스 2D 쿼드를 배치로 모아 드로우콜 최소화
//
// 사용 흐름:
//   1) Create()   — 장치 초기화 (Dynamic VB/IB, 흰 텍스처)
//   2) Clear()    — 매 프레임 시작 시 누적 초기화
//   3) AddQuad()  — 픽셀 좌표로 쿼드 누적 (내부에서 NDC 변환)
//   4) Flush()    — VB/IB 업로드 + 텍스처 배치별 DrawIndexed
//   5) Release()  — DX 리소스 해제
class FUIBatcher
{
public:
    FUIBatcher()  = default;
    ~FUIBatcher() = default;

    void Create(ID3D11Device* InDevice);
    void Release();

    // 픽셀 좌표 기준으로 쿼드 추가
    // Texture == nullptr 이면 Color 단색 쿼드로 처리
    void AddQuad(float ScreenX,   float ScreenY,
                 float Width,     float Height,
                 float ViewportW, float ViewportH,
                 UTexture*   Texture,
                 float R, float G, float B, float A);

    void Clear();
    void Flush(ID3D11DeviceContext* Context, const FRenderBus* RenderBus);

    uint32 GetQuadCount() const { return static_cast<uint32>(Vertices.size() / 4); }

private:
    TArray<FUIVertex> Vertices;
    TArray<uint32>    Indices;
    TArray<FUIBatch>  Batches;

    TComPtr<ID3D11Buffer> VertexBuffer;
    TComPtr<ID3D11Buffer> IndexBuffer;

    uint32 MaxVertexCount = 0;
    uint32 MaxIndexCount  = 0;

    TComPtr<ID3D11Device>             Device;
    UMaterialInterface*               Material = nullptr;

    // 단색 쿼드(Texture == nullptr)용 1x1 흰 SRV — UTexture 소유권 없이 직접 관리
    TComPtr<ID3D11ShaderResourceView> WhiteSRV;

    void CreateBuffers();
    void CreateWhiteTexture();
};
