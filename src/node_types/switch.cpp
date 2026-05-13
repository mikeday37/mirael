#include "pch.h"

#include <utility>

#include "imgui.h"
#include "ine/imgui_node_editor.h"
#include "ine/utilities/widgets.h"

#include "app.h"
#include "data.h"
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
    choicePinId = 0;
    if (dynamic)
        handleToggleDynamic();

    inputs.reserve(inputCount);
    for (int n : std::views::iota(1, inputCount + 1)) {
        addSwitchInputPin(n);
    }

    outPinId = addPin("out", {.direction = PinDirection::Output});
}

namespace
{

ImVec2 HeaderMin, HeaderMax;

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

void drawPin()
{
    const auto &style = App::get().getStyle();
    ax::Widgets::Icon({style.values.pinIconSize, style.values.pinIconSize}, ax::Drawing::IconType::Circle, false,
                      style.colors.pinIconColor);
}

void postPin() { ne::PopStyleVar(2); }

void postNode(NodeId id)
{
    if (!ImGui::IsItemVisible())
        return;

    auto &style = App::get().getStyle();

    auto itemMin = ImGui::GetItemRectMin();
    auto itemMax = ImGui::GetItemRectMax();

    auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

    auto drawList = ne::GetNodeBackgroundDrawList(id);

    const auto halfBorderWidth = ne::GetStyle().NodeBorderWidth * 0.5f;

    auto ul = ImVec2(itemMin.x + halfBorderWidth, itemMin.y + halfBorderWidth);
    auto lr = ImVec2(itemMax.x - halfBorderWidth, HeaderMax.y);
    drawList->AddRectFilled(ul, lr, ImColor(style.colors.nodeHeaderFill), ne::GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);

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
                    raiseModified(ChangeImpact::NodeConfig);
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
                raiseModified(ChangeImpact::NodeConfig);
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
        j["choice"] = manualChoice;
}

void Switch::onShowProperties()
{
    bool changed = false;

    int priorInputCount = inputCount;
    ImGui::InputInt("Inputs", &inputCount);
    inputCount = std::clamp(inputCount, 1, 100);
    if (priorInputCount != inputCount) {
        changed = true;
        if (inputCount > priorInputCount)
            expandInputs();
        else {
            assert(inputCount < priorInputCount);
            reduceInputs();
        }
    }

    changed |= ImGui::Checkbox("Enabled", &enabled);

    bool preDynamic = dynamic;
    changed |= ImGui::Checkbox("Dynamic", &dynamic);
    if (preDynamic != dynamic)
        handleToggleDynamic();

    if (!dynamic)
        changed |= ImGui::InputInt("Manual Choice", &manualChoice);

    if (changed)
        raiseModified(ChangeImpact::NodeConfig);
}

void Switch::expandInputs()
{
    if (inputCount > inputs.size())
        inputs.reserve(inputCount);

    while (inputCount > inputs.size()) {
        int pinNum = static_cast<int>(inputs.size()) + 1;
        addSwitchInputPin(pinNum);
    }
}

void Switch::reduceInputs()
{
    auto priorSize = static_cast<int>(inputs.size());

    inputs.resize(inputCount);

    while (priorSize > inputCount) {
        int pinNum = priorSize--;
        removeSwitchInputPin(pinNum);
    }
}

void Switch::addSwitchInputPin(int pinNumber)
{
    std::string key = std::format("in{}", pinNumber);
    auto pinId      = addPin(key, {.direction = PinDirection::Input});
    inputs.emplace_back(pinNumber, pinId, key);
}

void Switch::removeSwitchInputPin(int pinNumber)
{
    std::string key = std::format("in{}", pinNumber);
    removePin(key);
}

void Switch::handleToggleDynamic()
{
    if (dynamic && !choicePinId) {
        choicePinId = addPin("choice", {.direction = PinDirection::Input});
    } else if (choicePinId && !dynamic) {
        removePin("choice");
        choicePinId = 0;
    } else {
        assert(false); // the above two cases should be the only cases we encounter
    }
}

}; // namespace Mirael::NodeTypes
