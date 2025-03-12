#include <imgui.h>
#include <implot.h>

#include "ui.h"
#include "ok_color.h"

void reformant::ui::setStyle(bool bStyleDark, float alpha,
                             float scalingFactor) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.ScaleAllSizes(scalingFactor);

    // light style from Pacôme Danhiez (user itamago)
    // https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] =
        ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] =
        ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    /*style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);*/
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] =
        ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    /*style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);*/
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] =
        ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] =
        ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    /*style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
    style.Colors[ImGuiCol_CloseButtonHovered] =
        ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonActive] =
        ImVec4(0.98f, 0.39f, 0.36f, 1.00f);*/
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] =
        ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] =
        ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDimBg] =
        ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    if (bStyleDark) {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            ImVec4& col = style.Colors[i];
            float H, S, V;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

            if (S < 0.1f) {
                V = 1.0f - V;
            }
            ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
            if (col.w < 1.00f) {
                col.w *= alpha;
            }
        }
    } else {
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            ImVec4& col = style.Colors[i];
            if (col.w < 1.00f) {
                col.x *= alpha;
                col.y *= alpha;
                col.z *= alpha;
                col.w *= alpha;
            }
        }
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImPlot::GetStyle().Colors[ImPlotCol_FrameBg] = ImVec4(0, 0, 0, 0);
    ImPlot::GetStyle().Colormap = ImPlotColormap_Plasma;
    ImPlot::GetStyle().UseGpuAcceleration = true;
}

void reformant::ui::setOutlineColor(const float rgb[4], float out[4]) {
    using namespace ok_color;

    constexpr float weight = 0.9f;

    Lab colorLab = linear_srgb_to_oklab({rgb[0], rgb[1], rgb[2]});

    if (colorLab.L > 0.5f) {
        colorLab.L = (weight * colorLab.L + (1 - weight) * 1.0f) / 2;
    } else {
        colorLab.L = (weight * colorLab.L + (1 - weight) * 0.0f) / 2;
    }

    RGB colorRgb = oklab_to_linear_srgb(colorLab);
    out[0] = colorRgb.r;
    out[1] = colorRgb.g;
    out[2] = colorRgb.b;
    out[3] = 0.8f;
}
