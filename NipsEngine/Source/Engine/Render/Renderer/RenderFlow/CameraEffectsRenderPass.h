#pragma once
#include "RenderPass.h"
#include <memory>

class FShaderBindingInstance;

class FCameraEffectsRenderPass : public FBaseRenderPass
{
public:
    bool Initialize() override;
    bool Release() override;

private:
    bool Begin(const FRenderPassContext* Context) override;
    bool DrawCommand(const FRenderPassContext* Context) override;
    bool End(const FRenderPassContext* Context) override;

private:
    bool bSkip = false;
    std::shared_ptr<FShaderBindingInstance> ShaderBinding;
};
