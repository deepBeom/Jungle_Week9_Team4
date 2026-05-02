#include <d3d11.h>
#include "UIBatcher.h"
#include "Core/CoreTypes.h"
#include "Core/ResourceManager.h"

void FUIBatcher::Create(ID3D11Device* InDevice)
{
    Device = InDevice;

    MaxVertexCount = 256;
    MaxIndexCount  = 384;
    CreateBuffers();
    CreateWhiteTexture();

    UMaterial* Mat = FResourceManager::Get().GetMaterial("UIMat");
    Mat->DepthStencilType = EDepthStencilType::NoDepth;   // UI는 항상 위에 그림
    Mat->BlendType        = EBlendType::AlphaBlend;
    Mat->RasterizerType   = ERasterizerType::SolidNoCull;
    Mat->SamplerType      = ESamplerType::EST_Linear;

    Material = Mat;
}

void FUIBatcher::CreateBuffers()
{
    VertexBuffer.Reset();
    IndexBuffer.Reset();

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage          = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth      = sizeof(FUIVertex) * MaxVertexCount;
    vbDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&vbDesc, nullptr, VertexBuffer.ReleaseAndGetAddressOf());

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage          = D3D11_USAGE_DYNAMIC;
    ibDesc.ByteWidth      = sizeof(uint32) * MaxIndexCount;
    ibDesc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Device->CreateBuffer(&ibDesc, nullptr, IndexBuffer.ReleaseAndGetAddressOf());
}

void FUIBatcher::CreateWhiteTexture()
{
    // 단색 쿼드(Texture == nullptr)를 위한 1x1 흰 텍스처 생성
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width            = 1;
    TexDesc.Height           = 1;
    TexDesc.MipLevels        = 1;
    TexDesc.ArraySize        = 1;
    TexDesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Usage            = D3D11_USAGE_IMMUTABLE;
    TexDesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    const uint32 WhitePixel = 0xFFFFFFFF;
    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem     = &WhitePixel;
    InitData.SysMemPitch = 4;

    ID3D11Texture2D* Tex = nullptr;
    if (SUCCEEDED(Device->CreateTexture2D(&TexDesc, &InitData, &Tex)))
    {
        Device->CreateShaderResourceView(Tex, nullptr, WhiteSRV.ReleaseAndGetAddressOf());
        Tex->Release();
    }
}

void FUIBatcher::Release()
{
    Clear();
    VertexBuffer.Reset();
    IndexBuffer.Reset();
    WhiteSRV.Reset();
    Device.Reset();
}

void FUIBatcher::AddQuad(float ScreenX,   float ScreenY,
                         float Width,     float Height,
                         float ViewportW, float ViewportH,
                         UTexture*   Texture,
                         float R, float G, float B, float A)
{
    // 픽셀 좌표 → NDC 변환
    // X: 0~ViewportW  →  -1~+1
    // Y: 0~ViewportH  →  +1~-1  (DirectX는 Y축 위가 +1)
    auto ToNDCX = [&](float PX) { return  (PX / ViewportW) * 2.f - 1.f; };
    auto ToNDCY = [&](float PY) { return -(PY / ViewportH) * 2.f + 1.f; };

    const float X0 = ToNDCX(ScreenX);
    const float X1 = ToNDCX(ScreenX + Width);
    const float Y0 = ToNDCY(ScreenY);
    const float Y1 = ToNDCY(ScreenY + Height);

    // 텍스처가 바뀌면 새 배치 시작 — SubUVBatcher와 동일한 배치 분류 방식
    if (Batches.empty() || Batches.back().Texture != Texture)
    {
        FUIBatch Batch;
        Batch.Texture     = Texture;
        Batch.IndexStart  = static_cast<uint32>(Indices.size());
        Batch.IndexCount  = 0;
        Batch.BaseVertex  = static_cast<int32>(Vertices.size());
        Batches.push_back(Batch);
    }

    const uint32 LocalBase = static_cast<uint32>(Vertices.size())
        - static_cast<uint32>(Batches.back().BaseVertex);

    // 좌상 → 우상 → 좌하 → 우하
    Vertices.push_back({ X0, Y0,  0.f, 0.f,  R, G, B, A });
    Vertices.push_back({ X1, Y0,  1.f, 0.f,  R, G, B, A });
    Vertices.push_back({ X0, Y1,  0.f, 1.f,  R, G, B, A });
    Vertices.push_back({ X1, Y1,  1.f, 1.f,  R, G, B, A });

    Indices.push_back(LocalBase + 0); Indices.push_back(LocalBase + 1); Indices.push_back(LocalBase + 2);
    Indices.push_back(LocalBase + 1); Indices.push_back(LocalBase + 3); Indices.push_back(LocalBase + 2);

    Batches.back().IndexCount += 6;
}

void FUIBatcher::Clear()
{
    Vertices.clear();
    Indices.clear();
    Batches.clear();
}

void FUIBatcher::Flush(ID3D11DeviceContext* Context, const FRenderBus* RenderBus)
{
    if (Vertices.empty() || !VertexBuffer || !IndexBuffer) return;

    // 버퍼 부족하면 2배로 재할당 — SubUVBatcher와 동일한 방식
    if (Vertices.size() > MaxVertexCount || Indices.size() > MaxIndexCount)
    {
        MaxVertexCount = static_cast<uint32>(Vertices.size()) * 2;
        MaxIndexCount  = static_cast<uint32>(Indices.size())  * 2;
        CreateBuffers();
    }

    D3D11_MAPPED_SUBRESOURCE Mapped = {};

    if (FAILED(Context->Map(VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) return;
    memcpy(Mapped.pData, Vertices.data(), sizeof(FUIVertex) * Vertices.size());
    Context->Unmap(VertexBuffer.Get(), 0);

    if (FAILED(Context->Map(IndexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) return;
    memcpy(Mapped.pData, Indices.data(), sizeof(uint32) * Indices.size());
    Context->Unmap(IndexBuffer.Get(), 0);

    const uint32 Stride = sizeof(FUIVertex);
    const uint32 Offset = 0;
    ID3D11Buffer* VBPtr = VertexBuffer.Get();
    Context->IASetVertexBuffers(0, 1, &VBPtr, &Stride, &Offset);
    Context->IASetIndexBuffer(IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UMaterial* Mat = Cast<UMaterial>(Material);

    for (const FUIBatch& Batch : Batches)
    {
        if (Batch.IndexCount == 0) continue;

        // Texture == nullptr 이면 흰 SRV로 단색 처리
        ID3D11ShaderResourceView* SRV = (Batch.Texture)
            ? Batch.Texture->GetSRV()
            : WhiteSRV.Get();

        Context->PSSetShaderResources(0, 1, &SRV);
        Material->Bind(Context, RenderBus);

        Context->DrawIndexed(Batch.IndexCount, Batch.IndexStart, Batch.BaseVertex);
    }
}
