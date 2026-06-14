#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "ine/utilities/widgets.h"

#include "App.h"
#include "ImGuiEx.h"
#include "NodeEditorEx.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeEditorEx
{

void DrawPinIcon(bool alignToFramePadding)
{
    const auto &appStyle = App::get().getStyle();

    ImVec2 pos1 = ImGui::GetCursorPos();
    if (alignToFramePadding) {
        ImVec2 prePos = pos1;
        prePos.y += ImGui::GetStyle().FramePadding.y;
        ImGui::SetCursorPos(prePos);
    }

    ax::Widgets::Icon({appStyle.values.pinIconSize, appStyle.values.pinIconSize}, ax::Drawing::IconType::Circle, false,
                      appStyle.colors.pinIconColor);

    if (alignToFramePadding) {
        ImVec2 pos2    = ImGui::GetCursorPos();
        ImVec2 postPos = {pos2.x, pos1.y};
        ImGui::SetCursorPos(postPos);
    }
}

namespace StandardNodeHelper
{

void Builder::begin()
{
    const auto &style   = ImGui::GetStyle();
    pinDecorationWidth_ = App::get().getStyle().values.pinIconSize + style.ItemSpacing.x;

    ne::BeginNode(node_.getId());
    ImGui::PushID(node_.getIdAsPointer());
}

void Builder::preHeader()
{
    ImGui::Dummy({App::get().getStyle().values.nodeHeaderIndent, 0});
    ImGui::SameLine(0, 0);

    preHeaderX_ = ImGui::GetCursorScreenPos().x;
}

void Builder::postHeader()
{
    headerContentWidth_ = ImGui::GetItemRectMax().x - preHeaderX_;

    const auto &style = ImGui::GetStyle();
    ImGui::Dummy({0, style.ItemSpacing.y / 2.0f});
    headerMin_ = ImGui::GetItemRectMin();
    headerMax_ = ImGui::GetItemRectMax();
    ImGui::Dummy({0, style.ItemSpacing.y / 2.0f});
}

void Builder::prePin(PinId id, PinDirection dir)
{
    const bool input = dir == PinDirection::Input;
    ne::PushStyleVar(ne::StyleVar_PivotAlignment, input ? ImVec2(0, 0.5f) : ImVec2(1.0f, 0.5f));
    ne::PushStyleVar(ne::StyleVar_PivotSize, ImVec2(0, 0));

    ne::BeginPin(id, input ? ne::PinKind::Input : ne::PinKind::Output);

    if (input) {
        drawIcon();
        ImGui::SameLine();
    }
}

void Builder::postPin(PinDirection dir)
{
    if (dir == PinDirection::Output) {
        ImGui::SameLine();
        drawIcon();
    }

    ne::EndPin();
    ne::PopStyleVar(2);
}

void Builder::missingPin(float width) { ImGui::Dummy({width + pinDecorationWidth_, 0}); }

void Builder::spacing(float width) { ImGui::Dummy({width, 0}); }

void Builder::end()
{
    ImGui::PopID();
    ne::EndNode();

    if (!ImGui::IsItemVisible())
        return;

    auto &style = App::get().getStyle();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();

    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

    auto drawList = ne::GetNodeBackgroundDrawList(node_.getId());

    const auto halfBorderWidth = ne::GetStyle().NodeBorderWidth * 0.5f;

    auto ul = ImVec2(itemMin.x + halfBorderWidth, itemMin.y + halfBorderWidth);
    auto lr = ImVec2(itemMax.x - halfBorderWidth, headerMax_.y);
    drawList->AddRectFilled(ul, lr, ImColor(style.colors.nodeHeaderFill), ne::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);

    drawList->AddLine(ImVec2(ul.x, lr.y - 0.5f), ImVec2(lr.x - 1, lr.y - 0.5f), ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
}

float Builder::getMiddleSpacing(bool hasInputs, float maxInputWidth, float extraMiddleWidth, float maxOutputWidth) const
{
    const auto &appStyle      = App::get().getStyle();
    float expectedMiddleWidth = (hasInputs ? appStyle.values.pinColumnSpacing : 0) + extraMiddleWidth;
    float expectedTotalWidth  = maxInputWidth + expectedMiddleWidth + maxOutputWidth + (hasInputs ? 2.0f : 1.0f) * pinDecorationWidth_;
    float headerExcessWidth   = headerContentWidth_ + appStyle.values.nodeHeaderIndent - expectedTotalWidth;
    if (headerExcessWidth > 0)
        return expectedMiddleWidth + headerExcessWidth;
    else
        return expectedMiddleWidth;
}

} // namespace StandardNodeHelper

} // namespace Mirael::NodeEditorEx
