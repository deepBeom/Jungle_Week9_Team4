float FEditorViewportOverlayWidget::RenderShadowAtlasWindow(int32 ViewportIndex, const FEditorViewportState& VS, const ImVec2& Pos)
{
    if (!VS.bShowStatShadowAtlas || !EditorEngine)
    {
        return 0.0f;
    }

    FSceneViewport& SceneViewport = EditorEngine->GetViewportLayout().GetSceneViewport(ViewportIndex);
    FRenderTargetSet* RenderTargets = SceneViewport.GetRenderTargetSet();

    constexpr float PreviewSize = 256.0f;

    ImGui::SetNextWindowPos(Pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.3f);

    char WinId[40];
    snprintf(WinId, sizeof(WinId), "##ShadowAtlasOverlay_%d", ViewportIndex);

    float WindowWidth = 0.0f;
    if (ImGui::Begin(WinId, nullptr, kStatFlags))
    {
        ImGui::TextColored(ColorPink, "Shadow Atlas Stat");
        ImGui::Separator();

        // ──────────── Directional ────────────
        ImGui::BeginGroup();
        {
            ImGui::TextColored(ColorYellow, "Directional Shadow Atlas");
            ImGui::TextColored(ColorPaleBlue, "- Cascades: %u", FShadowAtlasManager::DirectionalCascadeCount);
            ImGui::TextColored(ColorPaleBlue, "- Atlas: %ux%u",
                FShadowAtlasManager::DirectionalAtlasResolution,
                FShadowAtlasManager::DirectionalAtlasResolution);

            if (RenderTargets != nullptr && RenderTargets->DirectionalShadowSRV != nullptr)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(RenderTargets->DirectionalShadowSRV), ImVec2(PreviewSize, PreviewSize));

                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                const ImVec2 Min = ImGui::GetItemRectMin();
                const ImVec2 Max = ImGui::GetItemRectMax();

                DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 180));
                DrawAtlasGrid(DrawList, Min, Max, FShadowAtlasManager::DirectionalAtlasGridDimension);

                const TArray<FDirectionalAtlasSlotDesc>& CascadeSlots = FShadowAtlasManager::GetDirectionalCascadeSlots();
                for (const FDirectionalAtlasSlotDesc& Slot : CascadeSlots)
                {
                    const float X0 = Min.x + (static_cast<float>(Slot.X) / FShadowAtlasManager::DirectionalAtlasResolution) * PreviewSize;
                    const float Y0 = Min.y + (static_cast<float>(Slot.Y) / FShadowAtlasManager::DirectionalAtlasResolution) * PreviewSize;
                    const float X1 = Min.x + (static_cast<float>(Slot.X + Slot.Width) / FShadowAtlasManager::DirectionalAtlasResolution) * PreviewSize;
                    const float Y1 = Min.y + (static_cast<float>(Slot.Y + Slot.Height) / FShadowAtlasManager::DirectionalAtlasResolution) * PreviewSize;

                    DrawList->AddRect(ImVec2(X0, Y0), ImVec2(X1, Y1), IM_COL32(255, 220, 0, 220), 0.0f, 0, 2.0f);

                    char Label[16];
                    snprintf(Label, sizeof(Label), "C%u", Slot.CascadeIndex);
                    DrawList->AddText(ImVec2(X0 + 4.0f, Y0 + 4.0f), IM_COL32(255, 220, 0, 255), Label);
                }
            }
            else
            {
                ImGui::Dummy(ImVec2(PreviewSize, PreviewSize));
                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                const ImVec2 Min = ImGui::GetItemRectMin();
                const ImVec2 Max = ImGui::GetItemRectMax();
                DrawList->AddRectFilled(Min, Max, IM_COL32(0, 0, 0, 150));
                DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 100));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // ──────────── Spot ────────────
        ImGui::BeginGroup();
        {
            ImGui::TextColored(ColorOrange, "Spot Shadow Atlas");
            ImGui::TextColored(ColorPaleBlue, "- Active Shadows: %u", RenderTargets ? RenderTargets->SpotShadowCount : 0);
            ImGui::TextColored(ColorPaleBlue, "- Atlas: %ux%u",
                FShadowAtlasManager::SpotAtlasResolution,
                FShadowAtlasManager::SpotAtlasResolution);

            if (RenderTargets != nullptr && RenderTargets->SpotShadowSRV != nullptr)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(RenderTargets->SpotShadowSRV), ImVec2(PreviewSize, PreviewSize));

                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                const ImVec2 Min = ImGui::GetItemRectMin();
                const ImVec2 Max = ImGui::GetItemRectMax();

                DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 180));

                const float BaseCell = PreviewSize / static_cast<float>(FShadowAtlasManager::SpotAtlasCellsPerRow);
                for (uint32 Line = 1; Line < FShadowAtlasManager::SpotAtlasCellsPerRow; ++Line)
                {
                    const float X = Min.x + BaseCell * static_cast<float>(Line);
                    const float Y = Min.y + BaseCell * static_cast<float>(Line);
                    DrawList->AddLine(ImVec2(X, Min.y), ImVec2(X, Max.y), IM_COL32(255, 255, 255, 35));
                    DrawList->AddLine(ImVec2(Min.x, Y), ImVec2(Max.x, Y), IM_COL32(255, 255, 255, 35));
                }

                const TArray<FSpotAtlasSlotDesc>& ActiveSlots = FShadowAtlasManager::GetActiveSpotSlots();
                for (const FSpotAtlasSlotDesc& Slot : ActiveSlots)
                {
                    const float X0 = Min.x + (static_cast<float>(Slot.X) / FShadowAtlasManager::SpotAtlasResolution) * PreviewSize;
                    const float Y0 = Min.y + (static_cast<float>(Slot.Y) / FShadowAtlasManager::SpotAtlasResolution) * PreviewSize;
                    const float X1 = Min.x + (static_cast<float>(Slot.X + Slot.Width) / FShadowAtlasManager::SpotAtlasResolution) * PreviewSize;
                    const float Y1 = Min.y + (static_cast<float>(Slot.Y + Slot.Height) / FShadowAtlasManager::SpotAtlasResolution) * PreviewSize;

                    DrawList->AddRect(ImVec2(X0, Y0), ImVec2(X1, Y1), IM_COL32(0, 255, 120, 220), 0.0f, 0, 2.0f);

                    char Label[32];
                    snprintf(Label, sizeof(Label), "%u (%u)", Slot.TileIndex, Slot.Width);
                    DrawList->AddText(ImVec2(X0 + 4.0f, Y0 + 4.0f), IM_COL32(0, 255, 120, 255), Label);
                }
            }
            else
            {
                ImGui::Dummy(ImVec2(PreviewSize, PreviewSize));
                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                const ImVec2 Min = ImGui::GetItemRectMin();
                const ImVec2 Max = ImGui::GetItemRectMax();
                DrawList->AddRectFilled(Min, Max, IM_COL32(0, 0, 0, 150));
                DrawList->AddRect(Min, Max, IM_COL32(255, 255, 255, 100));
            }
        }
        ImGui::EndGroup();

        WindowWidth = ImGui::GetWindowSize().x;
    }
    ImGui::End();

    return WindowWidth;
}