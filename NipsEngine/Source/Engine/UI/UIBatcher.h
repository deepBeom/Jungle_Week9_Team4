#pragma once
#include "Render/Resource/VertexTypes.h"
#include "Render/Common/ComPtr.h"

struct
{

};
class FUIBatcher
{
public:
    FUIBatcher() = default;
    ~FUIBatcher() = default;

public:
    void Flush(ID3D11DeviceContext* Context, float ViewportWidth, float ViewportHeight);

private:
    TArray<FUIVertex> Vertices;
    TArray<uint32> Indices;
    TArray<FSRVBatch> Batches;

    TComPtr<ID3D11Buffer> VertexBuffer;
    TComPtr<ID3D11Buffer> IndexBuffer;
};
