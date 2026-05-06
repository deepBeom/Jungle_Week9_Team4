#include "CameraEffectsRenderPass.h"
#include "Core/ResourceManager.h"
#include "Render/Scene/RenderBus.h"

bool FCameraEffectsRenderPass::Initialize()
{
    return true;
}

bool FCameraEffectsRenderPass::Release()
{
    ShaderBinding.reset();
    return true;
}

bool FCameraEffectsRenderPass::Begin(const FRenderPassContext* Context)
{
    bSkip = false;

    const FCameraEffectSettings& Effects = Context->RenderBus->GetCameraEffects();
    const bool bAnyEffect =
        Effects.FadeAlpha > 0.0f ||
        Effects.LetterBoxRatio > 0.0f ||
        Effects.VignetteIntensity > 0.0f;

    if (!bAnyEffect || !PrevPassSRV)
    {
        // 효과가 없으면 이전 패스 결과를 그대로 통과
        OutSRV = PrevPassSRV;
        OutRTV = PrevPassRTV;
        bSkip = true;
        return true;
    }

    UShader* Shader = FResourceManager::Get().GetShader("Shaders/Multipass/CameraEffectsPass.hlsl");
    if (!Shader)
    {
        OutSRV = PrevPassSRV;
        OutRTV = PrevPassRTV;
        bSkip = true;
        return true;
    }

    if (!ShaderBinding || ShaderBinding->GetShader() != Shader)
    {
        ShaderBinding = Shader->CreateBindingInstance(Context->Device);
    }

    if (!ShaderBinding)
    {
        OutSRV = PrevPassSRV;
        OutRTV = PrevPassRTV;
        bSkip = true;
        return true;
    }

    // 입력: 이전 패스 결과, 출력: SceneColor (ping-pong)
    ShaderBinding->SetSRV("SceneColor", PrevPassSRV);
    ShaderBinding->SetAllSamplers(FResourceManager::Get().GetOrCreateSamplerState(ESamplerType::EST_Linear));

    const float W = Context->RenderTargets->Width;
    const float H = Context->RenderTargets->Height;
    const float AspectRatio = (H > 0.0f) ? (W / H) : 1.0f;

    ShaderBinding->SetVector3("FadeColor", Effects.FadeColor);
    ShaderBinding->SetFloat("FadeAlpha", Effects.FadeAlpha);
    ShaderBinding->SetFloat("LetterBoxRatio", Effects.LetterBoxRatio);
    ShaderBinding->SetFloat("CurrentAspectRatio", AspectRatio);
    ShaderBinding->SetFloat("VignetteIntensity", Effects.VignetteIntensity);
    ShaderBinding->SetFloat("VignetteRadius", Effects.VignetteRadius);
    ShaderBinding->SetFloat("VignetteSoftness", Effects.VignetteSoftness);

    // SceneColorRTV는 Fog→FXAA 이후 해제된 상태이므로 안전하게 재사용
    ID3D11RenderTargetView* RTVs[1] = { Context->RenderTargets->SceneColorRTV };
    Context->DeviceContext->OMSetRenderTargets(1, RTVs, nullptr);

    Context->DeviceContext->IASetInputLayout(nullptr);
    Context->DeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    Context->DeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    Context->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    OutSRV = Context->RenderTargets->SceneColorSRV;
    OutRTV = Context->RenderTargets->SceneColorRTV;
    return true;
}

bool FCameraEffectsRenderPass::DrawCommand(const FRenderPassContext* Context)
{
    if (bSkip || !ShaderBinding)
    {
        return true;
    }

    ShaderBinding->Bind(Context->DeviceContext);
    Context->DeviceContext->Draw(3, 0);
    return true;
}

bool FCameraEffectsRenderPass::End(const FRenderPassContext* Context)
{
    if (bSkip)
    {
        return true;
    }

    ID3D11ShaderResourceView* NullSRVs[] = { nullptr };
    Context->DeviceContext->PSSetShaderResources(0, 1, NullSRVs);
    return true;
}
