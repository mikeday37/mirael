#include "pch.h"

#include <utility>

#include "imgui.h"
#include "imgui_node_editor.h"
#include "utilities/widgets.h"

#include "switch.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Switch::onDeserialize(const nlohmann::json &j)
{
    if (j.empty())
        return;

    enabled    = j["enabled"].get<bool>();
    dynamic    = j["dynamic"].get<bool>();
    inputCount = j["n"].get<int>();
    if (j.contains("choice"))
        manualChoice = j["choice"].get<int>();
}

void Switch::onInit()
{
    choicePinId = getPinId("choice");
    inputs.reserve(inputCount);
    for (int n : std::views::iota(1, inputCount + 1)) {
        std::string key   = std::format("in{}", n);
        inputs.emplace_back(n, getPinId(key), key);
    }
    outPinId = getPinId("out");
}

namespace
{

ImVec2 HeaderMin, HeaderMax;
ImU32 HeaderColor = IM_COL32(31, 191, 63, 127);

void postHeader()
{
    HeaderMin = ImGui::GetItemRectMin();
    HeaderMax = ImGui::GetItemRectMax();
}

void prePin(bool input = true)
{
    ne::PushStyleVar(ne::StyleVar_PivotAlignment, input ? ImVec2(0, 0.5f) : ImVec2(1.0f, 0.5f));
    ne::PushStyleVar(ne::StyleVar_PivotSize, ImVec2(0, 0));
}

void drawPin() { ax::Widgets::Icon({24, 24}, ax::Drawing::IconType::Circle, false, {0.5f, 0.5f, 0.5f, 1}); }

void postPin() { ne::PopStyleVar(2); }

void postNode(NodeId id)
{
    if (!ImGui::IsItemVisible())
        return;

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();

    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

    auto drawList = ne::GetNodeBackgroundDrawList(id);

    const auto halfBorderWidth = ne::GetStyle().NodeBorderWidth * 0.5f;

    auto ul = ImVec2(itemMin.x + halfBorderWidth, itemMin.y + halfBorderWidth);
    auto lr = ImVec2(itemMax.x - halfBorderWidth, HeaderMax.y);
    drawList->AddRectFilled(ul, lr, HeaderColor, ne::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);

    drawList->AddLine(ImVec2(ul.x, lr.y - 0.5f), ImVec2(lr.x - 1, lr.y - 0.5f), ImColor(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
}

}; // namespace

void Switch::onShow()
{
    auto &style = ImGui::GetStyle();

    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::Dummy({style.ItemInnerSpacing.x / 2.0f, 0});
    ImGui::SameLine();
    ImGui::TextUnformatted("Switch");
    ImGui::Dummy({0, style.ItemSpacing.y / 2.0f});
    postHeader();
    ImGui::Dummy({0, style.ItemSpacing.y / 2.0f});

    // optional choice input gets its own row
    if (dynamic) {
        prePin();
        ne::BeginPin(choicePinId, ne::PinKind::Input);
        drawPin();
        ImGui::SameLine();
        ImGui::TextUnformatted("choice");
        ne::EndPin();
        postPin();
    }

    // loop through inputs
    bool first = true;
    for (const auto &[n, id, label] : inputs) {
        prePin();
        ne::BeginPin(id, ne::PinKind::Input);
        drawPin();
        ImGui::SameLine();
        if (dynamic)
            ImGui::TextUnformatted(label.c_str());
        else {
            if (ImGui::RadioButton(label.c_str(), manualChoice == n)) {
                if (manualChoice != n)
                    raiseModified();
                manualChoice = n;
            }
        }
        ne::EndPin();
        postPin();

        // output will be aligned with the first input
        if (first) {
            first = false;
            ImGui::SameLine();
            ImGui::Dummy({ImGui::CalcTextSize("W").x * 2.0f, 0});
            ImGui::SameLine();

            prePin(false);
            ne::BeginPin(outPinId, ne::PinKind::Output);
            if (ImGui::Checkbox("out", &enabled))
                raiseModified();
            ImGui::SameLine();
            drawPin();
            ne::EndPin();
            postPin();
        }
    }

    ImGui::PopID();
    ne::EndNode();
    postNode(getId());
}

void Switch::onSerialize(nlohmann::json &j) const
{
    j["enabled"] = enabled;
    j["dynamic"] = dynamic;
    j["n"]       = inputCount;
    if (manualChoice != 0)
        j["choice"]  = manualChoice;
}

}; // namespace Mirael::NodeTypes
