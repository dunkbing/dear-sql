#pragma once
#include "imgui.h"

namespace Theme {
    struct Colors {
        ImVec4 rosewater;
        ImVec4 flamingo;
        ImVec4 pink;
        ImVec4 mauve;
        ImVec4 red;
        ImVec4 maroon;
        ImVec4 peach;
        ImVec4 yellow;
        ImVec4 green;
        ImVec4 teal;
        ImVec4 sky;
        ImVec4 sapphire;
        ImVec4 blue;
        ImVec4 lavender;
        ImVec4 text;
        ImVec4 subtext1;
        ImVec4 subtext0;
        ImVec4 overlay2;
        ImVec4 overlay1;
        ImVec4 overlay0;
        ImVec4 surface2;
        ImVec4 surface1;
        ImVec4 surface0;
        ImVec4 base;
        ImVec4 mantle;
        ImVec4 crust;
    };

    // Catppuccin Mocha (Dark)
    const Colors MOCHA = {
        ImVec4(0.957f, 0.839f, 0.839f, 1.0f), // rosewater
        ImVec4(0.929f, 0.788f, 0.808f, 1.0f), // flamingo
        ImVec4(0.957f, 0.765f, 0.824f, 1.0f), // pink
        ImVec4(0.910f, 0.765f, 0.957f, 1.0f), // mauve
        ImVec4(0.937f, 0.671f, 0.659f, 1.0f), // red
        ImVec4(0.937f, 0.678f, 0.737f, 1.0f), // maroon
        ImVec4(0.957f, 0.792f, 0.694f, 1.0f), // peach
        ImVec4(0.957f, 0.890f, 0.694f, 1.0f), // yellow
        ImVec4(0.647f, 0.902f, 0.671f, 1.0f), // green
        ImVec4(0.647f, 0.902f, 0.812f, 1.0f), // teal
        ImVec4(0.651f, 0.886f, 0.957f, 1.0f), // sky
        ImVec4(0.510f, 0.839f, 0.957f, 1.0f), // sapphire
        ImVec4(0.573f, 0.769f, 0.957f, 1.0f), // blue
        ImVec4(0.737f, 0.780f, 0.957f, 1.0f), // lavender
        ImVec4(0.859f, 0.859f, 0.898f, 1.0f), // text
        ImVec4(0.808f, 0.800f, 0.847f, 1.0f), // subtext1
        ImVec4(0.737f, 0.722f, 0.788f, 1.0f), // subtext0
        ImVec4(0.627f, 0.612f, 0.690f, 1.0f), // overlay2
        ImVec4(0.573f, 0.557f, 0.631f, 1.0f), // overlay1
        ImVec4(0.518f, 0.502f, 0.576f, 1.0f), // overlay0
        ImVec4(0.427f, 0.412f, 0.482f, 1.0f), // surface2
        ImVec4(0.376f, 0.361f, 0.431f, 1.0f), // surface1
        ImVec4(0.325f, 0.310f, 0.376f, 1.0f), // surface0
        ImVec4(0.110f, 0.110f, 0.137f, 1.0f), // base
        ImVec4(0.094f, 0.094f, 0.118f, 1.0f), // mantle
        ImVec4(0.078f, 0.078f, 0.098f, 1.0f)  // crust
    };

    // Catppuccin Latte (Light)
    const Colors LATTE = {
        ImVec4(0.859f, 0.557f, 0.541f, 1.0f), // rosewater
        ImVec4(0.839f, 0.475f, 0.482f, 1.0f), // flamingo
        ImVec4(0.529f, 0.463f, 0.847f, 1.0f), // pink
        ImVec4(0.529f, 0.463f, 0.847f, 1.0f), // mauve
        ImVec4(0.820f, 0.263f, 0.263f, 1.0f), // red
        ImVec4(0.827f, 0.325f, 0.376f, 1.0f), // maroon
        ImVec4(0.980f, 0.549f, 0.329f, 1.0f), // peach
        ImVec4(0.847f, 0.655f, 0.125f, 1.0f), // yellow
        ImVec4(0.400f, 0.671f, 0.384f, 1.0f), // green
        ImVec4(0.231f, 0.639f, 0.588f, 1.0f), // teal
        ImVec4(0.039f, 0.537f, 0.737f, 1.0f), // sky
        ImVec4(0.149f, 0.576f, 0.761f, 1.0f), // sapphire
        ImVec4(0.141f, 0.424f, 0.796f, 1.0f), // blue
        ImVec4(0.467f, 0.475f, 0.729f, 1.0f), // lavender
        ImVec4(0.169f, 0.169f, 0.169f, 1.0f), // text
        ImVec4(0.282f, 0.278f, 0.333f, 1.0f), // subtext1
        ImVec4(0.337f, 0.333f, 0.361f, 1.0f), // subtext0
        ImVec4(0.424f, 0.420f, 0.443f, 1.0f), // overlay2
        ImVec4(0.478f, 0.475f, 0.494f, 1.0f), // overlay1
        ImVec4(0.533f, 0.529f, 0.545f, 1.0f), // overlay0
        ImVec4(0.878f, 0.878f, 0.878f, 1.0f), // surface2
        ImVec4(0.925f, 0.925f, 0.925f, 1.0f), // surface1
        ImVec4(0.973f, 0.973f, 0.973f, 1.0f), // surface0
        ImVec4(0.957f, 0.957f, 0.957f, 1.0f), // base
        ImVec4(0.941f, 0.941f, 0.941f, 1.0f), // mantle
        ImVec4(0.925f, 0.925f, 0.925f, 1.0f)  // crust
    };

    void ApplyTheme(const Colors& colors) {
        ImGuiStyle& style = ImGui::GetStyle();

        // Window styling
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(5.0f, 2.0f);
        style.CellPadding = ImVec2(6.0f, 6.0f);
        style.ItemSpacing = ImVec2(6.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 21.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;

        // Border styling
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;

        // Rounding
        style.WindowRounding = 6.0f;
        style.ChildRounding = 6.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        // Alignment
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        // Colors
        ImVec4* colors_array = style.Colors;
        colors_array[ImGuiCol_Text] = colors.text;
        colors_array[ImGuiCol_TextDisabled] = colors.subtext0;
        colors_array[ImGuiCol_WindowBg] = colors.base;
        colors_array[ImGuiCol_ChildBg] = colors.mantle;
        colors_array[ImGuiCol_PopupBg] = colors.surface0;
        colors_array[ImGuiCol_Border] = colors.overlay0;
        colors_array[ImGuiCol_BorderShadow] = colors.crust;
        colors_array[ImGuiCol_FrameBg] = colors.surface0;
        colors_array[ImGuiCol_FrameBgHovered] = colors.surface1;
        colors_array[ImGuiCol_FrameBgActive] = colors.surface2;
        colors_array[ImGuiCol_TitleBg] = colors.mantle;
        colors_array[ImGuiCol_TitleBgActive] = colors.crust;
        colors_array[ImGuiCol_TitleBgCollapsed] = colors.surface0;
        colors_array[ImGuiCol_MenuBarBg] = colors.surface0;
        colors_array[ImGuiCol_ScrollbarBg] = colors.surface0;
        colors_array[ImGuiCol_ScrollbarGrab] = colors.overlay0;
        colors_array[ImGuiCol_ScrollbarGrabHovered] = colors.overlay1;
        colors_array[ImGuiCol_ScrollbarGrabActive] = colors.overlay2;
        colors_array[ImGuiCol_CheckMark] = colors.blue;
        colors_array[ImGuiCol_SliderGrab] = colors.blue;
        colors_array[ImGuiCol_SliderGrabActive] = colors.sapphire;
        colors_array[ImGuiCol_Button] = colors.surface0;
        colors_array[ImGuiCol_ButtonHovered] = colors.surface1;
        colors_array[ImGuiCol_ButtonActive] = colors.surface2;
        colors_array[ImGuiCol_Header] = colors.surface0;
        colors_array[ImGuiCol_HeaderHovered] = colors.surface1;
        colors_array[ImGuiCol_HeaderActive] = colors.surface2;
        colors_array[ImGuiCol_Separator] = colors.overlay0;
        colors_array[ImGuiCol_SeparatorHovered] = colors.overlay1;
        colors_array[ImGuiCol_SeparatorActive] = colors.overlay2;
        colors_array[ImGuiCol_ResizeGrip] = colors.overlay0;
        colors_array[ImGuiCol_ResizeGripHovered] = colors.overlay1;
        colors_array[ImGuiCol_ResizeGripActive] = colors.overlay2;
        colors_array[ImGuiCol_Tab] = colors.surface0;
        colors_array[ImGuiCol_TabHovered] = colors.surface1;
        colors_array[ImGuiCol_TabActive] = colors.surface2;
        colors_array[ImGuiCol_TabUnfocused] = colors.surface0;
        colors_array[ImGuiCol_TabUnfocusedActive] = colors.surface1;
        colors_array[ImGuiCol_DockingPreview] = colors.blue;
        colors_array[ImGuiCol_DockingEmptyBg] = colors.surface0;
        colors_array[ImGuiCol_PlotLines] = colors.text;
        colors_array[ImGuiCol_PlotLinesHovered] = colors.blue;
        colors_array[ImGuiCol_PlotHistogram] = colors.text;
        colors_array[ImGuiCol_PlotHistogramHovered] = colors.blue;
        colors_array[ImGuiCol_TableHeaderBg] = colors.surface0;
        colors_array[ImGuiCol_TableBorderStrong] = colors.overlay2;
        colors_array[ImGuiCol_TableBorderLight] = colors.overlay0;
        colors_array[ImGuiCol_TableRowBg] = colors.mantle;
        colors_array[ImGuiCol_TableRowBgAlt] = colors.crust;
        colors_array[ImGuiCol_TextSelectedBg] = colors.surface0;
        colors_array[ImGuiCol_DragDropTarget] = colors.green;
        colors_array[ImGuiCol_NavHighlight] = colors.blue;
        colors_array[ImGuiCol_NavWindowingHighlight] = colors.blue;
        colors_array[ImGuiCol_NavWindowingDimBg] = colors.overlay0;
        colors_array[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.35f);
    }
}
