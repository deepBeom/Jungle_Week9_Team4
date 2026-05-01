#include "EditorRenderPass.h"
#include "Render/LineBatcher.h"

bool FEditorRenderPass::Initialize()
{
    return true;
}

bool FEditorRenderPass::Release()
{
    return true;
}

bool FEditorRenderPass::Begin(const FRenderPassContext* Context)
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

bool FEditorRenderPass::DrawCommand(const FRenderPassContext* Context)
{
    if (Context->EditorLineBatcher == nullptr)
    {
        return true;
    }

    Context->EditorLineBatcher->Flush(Context->DeviceContext, Context->RenderBus);
    return true;
}

bool FEditorRenderPass::End(const FRenderPassContext* Context)
{
    return true;
}
