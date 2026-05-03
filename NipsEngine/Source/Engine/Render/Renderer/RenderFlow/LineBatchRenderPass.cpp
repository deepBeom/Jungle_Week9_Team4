#include "LineBatchRenderPass.h"
#include "Render/LineBatcher.h"

bool FLineBatchRenderPass::Initialize()
{
    return true;
}

bool FLineBatchRenderPass::Release()
{
    return true;
}

bool FLineBatchRenderPass::Begin(const FRenderPassContext* Context)
{
    ID3D11RenderTargetView* RTVs[3] = {
        PrevPassRTV ? PrevPassRTV : Context->RenderTargets->SceneColorRTV,
        Context->RenderTargets->SceneNormalRTV,
        Context->RenderTargets->SceneWorldPosRTV
    };
    ID3D11DepthStencilView* DSV = Context->RenderTargets->DepthStencilView;
    Context->DeviceContext->OMSetRenderTargets(ARRAYSIZE(RTVs), RTVs, DSV);

    OutSRV = PrevPassSRV;
    OutRTV = PrevPassRTV;
    return true;
}

bool FLineBatchRenderPass::DrawCommand(const FRenderPassContext* Context)
{
    if (Context->DebugLineBatcher == nullptr)
    {
        return true;
    }

    Context->DebugLineBatcher->Flush(Context->DeviceContext, Context->RenderBus);
    return true;
}

bool FLineBatchRenderPass::End(const FRenderPassContext* Context)
{
    return true;
}
